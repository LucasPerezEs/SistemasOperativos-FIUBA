#include <kern/history.h>
#include <inc/stdio.h>

extern int total_sched_calls;
int history_ind = 0;

// Updates history index.
// If max value is reached, it resets to the initial value.
void
update_history_ind()
{
	if (history_ind < HISTORY_LEN) {
		history_ind++;
	} else {
		history_ind = 0;
	}
}

// Stores id of the environment executed in history array correctly formatted.
void
add_run_env_to_history(struct Env *env)
{
	update_history_ind();
	snprintf(history[history_ind],
	         100,
	         "-Sched executes proccess %u\n",
	         env->env_id);
}

// Stores id of the environment created in history array correctly formatted.
void
add_new_env_to_history(struct Env *env)
{
	update_history_ind();

	snprintf(history[history_ind],
	         100,
	         "-Proccess created with id %u\n",
	         env->env_id);
}

// Stores information about environment after finishing in history array
// correctly formatted. Information stored: environment's id, number of times
// environment was run, and environment's time slices consumed.
void
add_end_of_env_to_history(struct Env *env)
{
	update_history_ind();
	snprintf(history[history_ind],
	         100,
	         "-Proccess %u ended with a total of %u times run and a total "
	         "of %u time slices consumed\n",
	         env->env_id,
	         env->env_runs,
	         env->time_slices);
}

// Prints scheduler statistics including:
// Total number of calls to the scheduler
// History of environments selected
// Number of runs and time slices per environment
void
print_history(void)
{
	cprintf("History\n");
	for (int i = 0; i < HISTORY_LEN; i++) {
		int index = (i + history_ind) % HISTORY_LEN;

		if (strlen(history[index]) != 0) {
			cprintf("%s", history[index]);
		}
	}
}