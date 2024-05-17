#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

char *
eliminar_salto(char *line)
{
	if (!line)
		return NULL;

	line[strcspn(line, "\n")] = 0;

	return line;
}

void
reiniciar_args(char *args[], char *argv_1)
{
	for (int i = 1; i < NARGS + 1; i++) {
		args[i] = NULL;
	}

	args[0] = argv_1;

	return;
}

int
main(int argc, char *argv[])
{
	char *line = NULL;
	size_t len = 0;
	int nread;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *args[NARGS + 2] = { 0 };

	args[0] =
	        argv[1];  // The first argument, by convention, should point  to
	                  // the filename associated with the file being executed.
	args[NARGS + 1] =
	        NULL;  // The array of pointers must be terminated by a null pointer.


	int i = 1;

	while ((nread = getline(&line, &len, stdin)) != -1) {
		line = eliminar_salto(line);

		args[i] = strdup(line);

		if (i % NARGS == 0) {
			int child_id = fork();

			if (child_id < 0) {
				printf("Error en fork.\n");
				free(line);
				exit(-1);
			}

			if (child_id == 0) {
				if (execvp(argv[1], args) < 0) {
					printf("Error en execvp.\n");
					free(line);
					exit(-1);
				}

				exit(0);

			} else {
				wait(NULL);
				i = 0;
				reiniciar_args(args, argv[1]);
			}
		}

		i++;
	}

	if (i > 1) {
		if (execvp(argv[1], args) < 0) {
			printf("Error en execvp.\n");
			free(line);
			exit(-1);
		}
	}

	for (int i = 0; i < NARGS + 1; i++) {
		free(args[i]);
	}


	free(line);
	exit(EXIT_SUCCESS);
}