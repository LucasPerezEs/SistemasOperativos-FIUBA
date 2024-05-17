#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <inc/string.h>
#include <kern/history.h>

#define MAX_PRIORITY 10
#define MAX_TIMESLICES_BOOST 1000
#define MAX_ENV_TIME_SLICES 3
int boost_time_slices = 0;
int total_sched_calls = 0;

void sched_halt(void);

// Implement simple round-robin scheduling.
//
// Search through 'envs' for an ENV_RUNNABLE environment in
// circular fashion starting just after the env this CPU was
// last running.  Switch to the first such environment found.
//
// If no envs are runnable, but the environment previously
// running on this CPU is still ENV_RUNNING, it's okay to
// choose that environment.
//
// Never choose an environment that's currently running on
// another CPU (env_status == ENV_RUNNING). If there are
// no runnable environments, simply drop through to the code
// below to halt the cpu.
void
sched_rr(void)
{
	int next_curenv_ind = 0;

	if (curenv) {
		next_curenv_ind = ENVX(curenv->env_id) + 1;
	}

	for (int i = 0; i < NENV; i++) {
		int ind = (i + next_curenv_ind) % NENV;
		struct Env *env_to_run = &envs[ind];
		if (env_to_run->env_status == ENV_RUNNABLE) {
			add_run_env_to_history(env_to_run);
			env_run(env_to_run);
		}
	}

	if (curenv && curenv->env_status == ENV_RUNNING &&
	    curenv->env_cpunum == thiscpu->cpu_id) {
		add_run_env_to_history(curenv);
		env_run(curenv);
	}
}


// Checks if total time slices reaches max value.
// If so, all environment's priorities are reset.
void
boost(void)
{
	boost_time_slices += 1;
	if (boost_time_slices == MAX_TIMESLICES_BOOST) {
		for (int i = 0; i < NENV; i++) {
			struct Env *actual_env = &envs[i];
			actual_env->priority = 1;
		}
		boost_time_slices = 1;
	}
}

// Updates time slices
// If time slices reaches max value, reduces current environment's priority.
void
update_priority(void)
{
	if (!curenv) {
		return;
	}

	curenv->time_slices++;

	if (((curenv->time_slices) % MAX_ENV_TIME_SLICES == 0) &&
	    curenv->priority < MAX_PRIORITY) {
		curenv->priority += 1;
	}

	boost();
}

// Search through 'envs' for an ENV_RUNNABLE environment in
// circular fashion starting just after the env this CPU was
// last running.  Switch to the environment with best priority.
//
// If an envionment with minimum priority is found, simply switch
// to such environment.
//
// If no envs are runnable, but the environment previously
// running on this CPU is still ENV_RUNNING, it's okay to
// choose that environment.
//
// Never choose an environment that's currently running on
// another CPU (env_status == ENV_RUNNING). If there are
// no runnable environments, simply drop through to the code
// below to halt the cpu.
void
sched_pr(void)
{
	int best_priority = MAX_PRIORITY + 1;
	struct Env *env_to_run = NULL;

	int next_curenv_ind = 0;

	if (curenv) {
		next_curenv_ind =
		        ENVX(curenv->env_id) +
		        1;  // ENVX es una macro que devuelve el indice del env en la lista de envs
	}

	for (int i = 0; i < NENV; i++) {
		int ind = (i + next_curenv_ind) % NENV;
		struct Env *actual_env = &envs[ind];

		if (actual_env->env_status == ENV_RUNNABLE &&
		    actual_env->priority == 1) {
			env_to_run = actual_env;
			break;
		}
		if (actual_env->env_status == ENV_RUNNABLE &&
		    actual_env->priority < best_priority) {
			env_to_run = actual_env;
			best_priority = actual_env->priority;
		}
	}

	if (curenv && curenv->env_status == ENV_RUNNING &&
	    curenv->env_cpunum == thiscpu->cpu_id && !env_to_run) {
		env_to_run = curenv;
	}

	if (env_to_run) {
		add_run_env_to_history(env_to_run);
		env_run(env_to_run);
	}
}

// Choose a user environment to run and run it.
//
// If SCHED_RR is defined, the environment is selected
// using a simple round-robin scheduling.
//
// If SCHED_PR is defined, the environment is selected
// using a priority scheduler.
void
sched_yield(void)
{
	// Your code here
	// Wihtout scheduler, keep runing the last environment while it exists

	total_sched_calls += 1;
#ifdef SCHED_RR
	sched_rr();
#endif

#ifdef SCHED_PR
	sched_pr();
#endif

	// sched_halt never returns
	sched_halt();
}

void
print_sched_stats()
{
	cprintf("\n\nSched stats:\n\n");
	cprintf("Number of calls to sched: %u\n\n", total_sched_calls);
	print_history();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		print_sched_stats();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
