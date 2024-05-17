#ifndef FISOPFS_H
#define FISOPFS_H

#define FUSE_USE_VERSION 30
#include <time.h>
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "entrada.h"
#include "entrada_archivo.h"
#include "entrada_directorio.h"
#include "utilidades.h"

#define MAXIMO_PATH 2048
#define ENTRADAS_INICIALES 5
#define MULTIPLICADOR_ENTRADAS 2
#define ARCHIVO_PERSISTENCIA_DEFAULT "persitencia.fisopfs"


extern long long unsigned MAX_ESPACIO_FS;
extern char *ARCHIVO_PERSISTENCIA;


enum { VACIO = 'V', ARCHIVO = 'A', DIRECTORIO = 'D' };

// Declaración de la estructura fs
typedef struct fisopfs_fs {
	struct entrada_directorio *raiz;
	size_t espacio_total_ocupado_fs;
} fisopfs_fs_t;

// Declaración de la variable global fs como externa
extern fisopfs_fs_t fs;

void crear_directorio_raiz(void);

/// Busca una entrada acorde al path, si el archivo no existe elimina
struct entrada *encontrar_entrada_rec(char *entrada_a_,
                                      const char *resto_del_path,
                                      struct entrada_directorio *directorio);

// Busca la entrada correspondiente al path en el fs. Si no existe
// devuelve NULL
struct entrada *encontrar_entrada(const char *path);


#endif