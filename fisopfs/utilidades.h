#ifndef UTILIDADES_H
#define UTILIDADES_H
#include "entrada_directorio.h"
#include "fisopfs.h"

// Recibe un path y devuelve el path hasta el ultimo '/', sin incluirlo.
// Ejemplo:
// Recibe: /usr/lib
// Devuelve: /usr
char *obtener_parte_directorio(char *path);

// Obtiene el ultimo directorio de un path
// Ejemplo:
// recibe: path = "/home/siro/sisop"
// devuelve: path_directorio = "/home/siro"
void obtener_directorio_path(const char *path, char path_directorio[MAXIMO_PATH]);

// Obteien el nombre final de un path
// Ejemplo:
// recive: path = "/home/siro/sisop"
// devuelve: path_directorio = "siro"
void obtener_nombre_final_path(const char *path,
                               char nombre_final_path[MAXIMO_PATH]);

// divide el path que recive por la mitad, acorde al primer caracter '/' que recive
// Ejemplos:
// recive: cadena = home/siro/fatala
// devuelve: primera_parte = home y segunda_parte = siro/fatala
//
// recive: cadena = archivesco
// devuelve: primera_parte = archivesco y segunda_parte = NULL
void dividir_path_en_dos(const char *cadena,
                         char *primera_parte,
                         const char **segunda_parte);


#endif