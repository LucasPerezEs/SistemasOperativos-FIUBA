#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t parent = sys_getenvid();

	for (int i = 0; i < 5; i++) {
		sys_red_priority(parent);
	}

	int r = fork();

	if (r == 0) {
		if (sys_get_priority(sys_getenvid()) == sys_get_priority(parent))
			cprintf("I am the child and my priority is the same as "
			        "my parent.\n");

	} else {
		sys_yield();
	}
}