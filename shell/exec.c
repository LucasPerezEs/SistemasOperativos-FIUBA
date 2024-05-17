#include "exec.h"

#define ERROR -1
// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	char key[BUFLEN];
	char value[BUFLEN];
	int idx;
	for (int i = 0; i < eargc; i++) {
		idx = block_contains(eargv[i], '=');
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, idx);
		setenv(key, value, 1);
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
// Pasarle O_CLOEXEC
static int
open_redir_fd(char *file, int flags)
{
	return open(file, flags, S_IWUSR | S_IRUSR);
}

// calls the function execvp(3) wrapper to execve(2) given an execcmd struct
// previously parsed with all the necessary data. This replaces the current
// process's image with the one given by the execcmd
static void
exec_command(struct execcmd *e)
{
	set_environ_vars(e->eargv, e->eargc);
	execvp(e->argv[0], e->argv);
	perror("Fallo en el exec");
	_exit(ERROR);
}

// given a file name, a file descriptor and flags it
// calls the dup2(2) syscall replacing the oldfd with a fd
// of file with the file name recieved which was opened within the function.
static void
dup_file(char file[FNAMESIZE], int oldfd, int flags)
{
	if (strlen(file) > 0) {
		int newfd = open_redir_fd(file, flags);

		if (dup2(newfd, oldfd) == ERROR) {
			_exit(ERROR);
		}
		close(newfd);
	}
}

// given a struct execcmd it redirects its stdin, stdout or stderr to
// the corresponding file previosly parsed given through the command line
static void
exec_redir(struct execcmd *r)
{
	dup_file(r->in_file, 0, O_CLOEXEC | O_RDONLY);

	dup_file(r->out_file, 1, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC);

	if (strlen(r->err_file) > 0) {
		if (r->err_file[0] == '&' && r->err_file[1] == '1') {
			if (dup2(1, 2) == ERROR) {
				perror("Fallo en el dup");
				_exit(ERROR);
			}
		} else {
			dup_file(r->err_file, 2, O_CREAT | O_WRONLY | O_CLOEXEC);
		}
	}

	r->type = EXEC;
	exec_cmd((struct cmd *) r);
	_exit(-1);
}

// given a struct pipecmd it forks and executes both commands one in each
// process. Then it redirects the sdtout of the left program to the stdin
// of the right program utilizing pipes and dup2(2) function
static void
exec_pipe(struct pipecmd *p)
{
	pid_t pid_r;
	pid_t pid_l;
	int pipefd[2];

	if (pipe(pipefd) == ERROR) {
		perror("Error al abrir la pipe");
		_exit(ERROR);
	}

	if ((pid_r = fork()) < 0) {
		perror("Error al hacer fork");
		_exit(ERROR);
	}

	if (pid_r == 0) {
		// hijo, ejecuto el cmd derecho
		close(pipefd[WRITE]);

		if (dup2(pipefd[READ], 0) < 0) {
			perror("Fallo el dup");
			_exit(ERROR);
		}

		close(pipefd[READ]);
		if (p->rightcmd->type == EXEC)
			print_status_info(p->rightcmd);

		exec_cmd(p->rightcmd);

	} else {
		// padre, ejecuto el cmd izquierdo

		if ((pid_l = fork()) == 0) {
			close(pipefd[READ]);

			if (dup2(pipefd[WRITE], 1) < 0) {
				perror("Fallo el dup");
				_exit(ERROR);
			}

			close(pipefd[WRITE]);

			exec_cmd(p->leftcmd);

		} else {
			close(pipefd[READ]);
			close(pipefd[WRITE]);
			waitpid(pid_r, NULL, 0);
			waitpid(pid_l, NULL, 0);
			_exit(0);
		}
	}
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	switch (cmd->type) {
	case EXEC: {
		// spawns a command
		setpgid(0, 0);
		exec_command((struct execcmd *) cmd);
		break;
	}

	case BACK: {
		// runs a command in background
		struct backcmd *b;
		b = (struct backcmd *) cmd;
		exec_command((struct execcmd *) b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		setpgid(0, 0);
		exec_redir((struct execcmd *) cmd);
		break;
	}

	case PIPE: {
		setpgid(0, 0);
		exec_pipe((struct pipecmd *) cmd);
		break;
	}
	}
}
