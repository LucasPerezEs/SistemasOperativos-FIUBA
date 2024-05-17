#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include "signal_handler.h"

char prompt[PRMTLEN] = { 0 };

// runs a shell command
static void
run_shell()
{
	char *cmd;
	size_t size;
	char *actual_path;
	while ((cmd = read_line(prompt)) != NULL) {
		actual_path = getcwd(NULL, size);
		snprintf(prompt, sizeof prompt, "(%s)", actual_path);
		free(actual_path);
		if (run_cmd(cmd) == EXIT_SHELL) {
			return;
		}
	}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		init_handler();
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	free_handler();

	return 0;
}
