#include <inc/lib.h>

#include <inc/syscall.h>
void
umain(int argc, char **argv)
{
	int initial_priority = sys_get_priority(sys_getenvid());
	while (1) {
		if (sys_get_priority(sys_getenvid()) - initial_priority == 5) {
			cprintf("Priority was reduced 5 points.\n");
			break;
		}
	}
}
