#include "utilidades.h"


char *
obtener_parte_directorio(char *path)
{
	size_t longitud = strlen(path);

	// Manejo caso especial: cadena vacia
	if (longitud == 0) {
		path[0] = '.';
		path[1] = '\0';
		return path;
	}

	// Manejo caso especial: path '/'
	if (path[0] == '/' && longitud == 1) {
		return path;
	}

	// Se busca la ultima barra desde el final de la cadena
	int i;
	for (i = longitud - 1; i >= 0 && path[i] != '/'; i--) {
	}

	// Verificar si se encontro una barra
	if (i >= 0) {
		// Se encontro una barra
		if (i == 0) {
			path[1] = '\0';  // Caso especial: el path es '/'
		} else {
			path[i] = '\0';  // Se reemplaza la ultima barra por el caracter nulo
		}

	} else {
		// Si no hay barra, el path es un archivo en el directorio
		path[0] = '.';
		path[1] = '\0';
	}

	return path;
}


void
obtener_directorio_path(const char *path, char path_directorio[MAXIMO_PATH])
{
	strcpy(path_directorio, path);
	char *archivo = obtener_parte_directorio(path_directorio);
	strcpy(path_directorio, archivo);
}

void
obtener_nombre_final_path(const char *path, char nombre_final_path[MAXIMO_PATH])
{
	const char *ultima_barra = strrchr(path, '/');

	if (ultima_barra != NULL) {
		strcpy(nombre_final_path, ultima_barra + 1);
	} else {
		strcpy(nombre_final_path, path);
	}
}


void
dividir_path_en_dos(const char *cadena,
                    char *primera_parte,
                    const char **segunda_parte)
{
	// Encontrar la posición del primer "/"
	const char *posicionBarra = strchr(cadena, '/');

	if (posicionBarra != NULL) {
		// Calcular la longitud de la primera parte
		size_t longitudPrimeraParte = posicionBarra - cadena;

		// Copiar la subcadena al búfer
		strncpy(primera_parte, cadena, longitudPrimeraParte);
		primera_parte[longitudPrimeraParte] = '\0';

		// Establecer la dirección de la segunda parte
		*segunda_parte = posicionBarra + 1;
	} else {
		// No se encontró "/", copiar la cadena completa al búfer
		strcpy(primera_parte, cadena);
		*segunda_parte = NULL;
	}
}
