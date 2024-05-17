#ifndef ENTRADA_DIRECTORIO_H
#define ENTRADA_DIRECTORIO_H

#include "fisopfs.h"
#include "entrada.h"


typedef struct entrada_directorio {
	char nombre[MAXIMO_PATH];
	struct stat st;
	char tipo;
	size_t cantidad_entradas;
	size_t cantidad_entradas_ocupadas;
	struct entrada **entradas;
} entrada_directorio_t;


// Llena una entrada vacia del directorio con la nueva entrada recivida
//(puede ser un archivo o directorio)
int colocar_entrada_dentro_de_directorio(entrada_directorio_t *directorio,
                                         struct entrada *nueva_entrada);


// Setea todos los campos del directorio resivido
int crear_dir(entrada_directorio_t *directorio, const char *path, mode_t mode);

// libera la memoria reservada por un directorio de forma recursiva
void liberar_dir(entrada_directorio_t *directorio);

// Expande la cantidad de entradas de un directorio disponibles.
// Si no tiene entradas se reserva espacio acorde a ENTRADAS_INICIALES
// Si ya tenia entradas se duplica el espacio. EN ambos casos las
// entradas nuevas se llenas con entradas vacias
int expandir_entrdas_dir(entrada_directorio_t *directorio);

// Crea un directorio apartir de la lectura de un archivo en disco
entrada_directorio_t *crear_directorio_desde_archivo(FILE *archivo);

// Escribe de forma recursiva el directorio en archivo para su permanencia en
// disco Las entradas vacias del directorio no se escriben en disco
int escribir_directorio_en_archivo(entrada_directorio_t *directorio,
                                   FILE *archivo);

// devuelve true si el directorio esta vacio, false caso contrario
bool esta_vacio_directorio(entrada_directorio_t *directorio);

#endif