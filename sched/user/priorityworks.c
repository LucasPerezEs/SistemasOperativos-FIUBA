#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int r = fork();

	if (r == 0) {
		int i = 0;
		while (1) {
			if (i == 1000) {
				cprintf("My priority is better, I reach 1000 "
				        "iterations faster.\n");
				break;
			}
			i++;
		}

	} else {
		for (int i = 0; i < 5; i++) {
			sys_red_priority(sys_getenvid());
		}

		sys_yield();

		int j = 0;
		while (1) {
			if (j == 1000) {
				cprintf("My priority is worse, I reach 1000 "
				        "iterations slower.\n");
				break;
			}
			j++;
		}
	}
}