#include "signal_handler.h"

stack_t ss;
struct sigaction sa;

// called when a signal of kind SIGCHILD is caught
// if the child that exited was a shell background process then it prints its pid
void
handler(int signum __attribute__((unused)))
{
	pid_t pgid = waitpid(0, &status, WNOHANG);
	if (pgid > 0) {
		char buf[BUFLEN];
		if (snprintf(buf, BUFLEN, "terminado: PID=%d\n$ ", pgid) < 0)
			perror("Error en snprintf, handler");
		if (write(1, buf, strlen(buf)) < 0)
			perror("Error al escribir, handler");
	}
	return;
}

// initializes the signal handler with sigaction()
// sets its flags to:
//      - SA_ONSTACK : tells the sigaction handler to use an alternative stack
//                     (set by sigaltstack()) instead of the current process's one.
//      - SA_RESTART : in case the signal interrupted any syscall's execution
//                     it lets it resume.
void
init_handler()
{
	sa.sa_flags = SA_ONSTACK | SA_RESTART;

	if (sigfillset(&sa.sa_mask) < 0) {
		perror("Error al settear las signals");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = handler;

	ss.ss_sp = malloc(SIGSTKSZ);
	if (ss.ss_sp == NULL) {
		perror("Error al reservar memoria para el stack secundario");
		exit(EXIT_FAILURE);
	}

	ss.ss_size = SIGSTKSZ;

	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL)) {
		perror("Error en sigaltstack");
		free(ss.ss_sp);
		exit(EXIT_FAILURE);
	}

	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("Error en sigaction");
		free(ss.ss_sp);
		exit(EXIT_FAILURE);
	}
}

// frees the alternate stack previously malloc'd in init_handler()
void
free_handler()
{
	free(ss.ss_sp);
}