#ifndef ENTRADA_ARCHIVO_H
#define ENTRADA_ARCHIVO_H

#include "fisopfs.h"
#define MAXIMO_PATH 2048

typedef struct entrada_archivo {
	char nombre[MAXIMO_PATH];
	struct stat st;
	char tipo;
	char *contenido;
} entrada_archivo_t;

// Setea todos los campos de directorio
int crear_archivo(entrada_archivo_t *archivo, char *nombre, mode_t modo);

// libera el archivo
void liberar_archivo(entrada_archivo_t *archivo);

// Crear un archivo apartir de la lectura de un archivo
entrada_archivo_t *crear_entrada_archivo_desde_archivo(FILE *archivo_a_leer);

// escribe el contenido de archivo en archivo_a_escribir
int escribir_entrada_archivo_en_archivo(entrada_archivo_t *archivo,
                                        FILE *archivo_a_escribir);

#endif