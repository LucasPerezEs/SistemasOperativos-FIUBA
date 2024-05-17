#ifndef ENTRADA_H
#define ENTRADA_H
#include <sys/stat.h>
#include "fisopfs.h"  // para incluir al filesystem (la variable global)

#define MAXIMO_PATH 2048

typedef struct entrada {
	char nombre[MAXIMO_PATH];
	struct stat st;
	char tipo;
} entrada_t;

// crea una entrada vacia y la devuelve
entrada_t *crear_entrada_vacia(void);

// llena las campos de stat con la entra recibida
void llenar_stat_entrada(entrada_t *entrada, struct stat *st);

#endif