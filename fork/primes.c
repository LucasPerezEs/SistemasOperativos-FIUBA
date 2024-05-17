#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

void
proceso_filtro(int pipe_izq[2])
{
	close(pipe_izq[WRITE]);

	int pipe_der[2];

	if (pipe(pipe_der) < 0) {
		printf("Error al crear pipe.\n");
		exit(-1);
	}

	int primo;
	if (read(pipe_izq[READ], &primo, sizeof(primo)) ==
	    0) {  // Cuando lee un 0 (end of file) termina.
		close(pipe_izq[READ]);
		close(pipe_izq[WRITE]);
		close(pipe_der[READ]);
		close(pipe_der[WRITE]);
		return;
	}

	printf("primo %d\n", primo);

	pid_t child_id = fork();

	if (child_id < 0) {
		printf("Error al hacer fork.\n");
		exit(-1);
	}

	if (child_id == 0) {
		close(pipe_izq[READ]);
		proceso_filtro(pipe_der);
		close(pipe_der[READ]);
		return;

	} else {
		close(pipe_der[READ]);

		int value;
		while (read(pipe_izq[READ], &value, sizeof(value)) > 0) {
			if (value % primo != 0) {
				if (write(pipe_der[WRITE], &value, sizeof(value)) <
				    0) {
					printf("Error al escribir en pipe.\n");
					exit(-1);
				}
			}
		}

		close(pipe_izq[READ]);
		close(pipe_der[WRITE]);

		wait(NULL);

		return;
	}
}


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("No se recibio n.\n");
		exit(-1);
	}

	int max = atoi(argv[1]);

	int pipe_izq[2];

	if (pipe(pipe_izq) < 0) {
		printf("Error en la creacion del pipe.\n");
		exit(-1);
	}

	pid_t child_id = fork();

	if (child_id < 0) {
		printf("Error al realizar fork.\n");
		exit(-1);
	}

	if (child_id == 0) {
		// HIJO
		proceso_filtro(pipe_izq);

	} else {
		// PADRE (GENERADOR)

		close(pipe_izq[READ]);

		for (int i = 2; i <= max; i++) {
			if (write(pipe_izq[WRITE], &i, sizeof(i)) < 0) {
				printf("Error al escribir en pipe.\n");
				exit(-1);
			}
		}

		close(pipe_izq[WRITE]);

		wait(NULL);
	}


	return 0;
}