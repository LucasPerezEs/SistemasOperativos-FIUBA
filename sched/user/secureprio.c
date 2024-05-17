// checks if auto-increasing priority is prohibited

#include <inc/lib.h>
#define ERROR -1

void
umain(int argc, char **argv)
{
	envid_t parent = sys_getenvid();
	int initial_prio = sys_get_priority(sys_getenvid());
	int r = sys_inc_priority(sys_getenvid());

	if (r == ERROR && sys_get_priority(sys_getenvid()) == initial_prio)
		cprintf("I can't increase my own priority...\n");

	int id = fork();
	if (id == 0) {
		int r2 = sys_inc_priority(parent);
		if (r2 == ERROR && sys_get_priority(parent) == initial_prio)
			cprintf("My child can't increase my priority...");

		cprintf("Parent's priority is %d");

	} else {
		sys_yield();
	}
}