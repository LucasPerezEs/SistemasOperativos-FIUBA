#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0)
		return 1;
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strstr(cmd, "cd")) {
		int result;
		char *actual_path;
		size_t size;
		if (block_contains(cmd, ' ') > 0) {
			char *dir = split_line(cmd, ' ');
			result = chdir(dir);
		} else
			result = chdir(getenv("HOME"));

		if (result < 0)
			return 0;  // no se encontro el dir
		else {
			actual_path = getcwd(NULL, size);
			snprintf(prompt, sizeof prompt, "(%s)", actual_path);
			free(actual_path);
			return 1;
		}
	}
	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		size_t size;
		char *actual_path = getcwd(NULL, size);
		if (actual_path != NULL) {
			printf("%s\n", actual_path);
			free(actual_path);
			return 1;
		}
	}
	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd __attribute__((unused)))
{
	return 0;
}
