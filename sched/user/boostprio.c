#include <inc/lib.h>

#include <inc/syscall.h>

void
umain(int argc, char **argv)
{
	int initial_priority = sys_get_priority(sys_getenvid());
	envid_t curenvid = sys_getenvid();
	sys_red_priority(curenvid);

	while (1) {
		if (initial_priority == sys_get_priority(sys_getenvid())) {
			cprintf("My priority after BOOST was restablished.\n");
			break;
		}
	}
}
