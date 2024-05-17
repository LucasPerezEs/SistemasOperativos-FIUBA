#include "entrada.h"


entrada_t *
crear_entrada_vacia()
{
	entrada_t *entrada = malloc(sizeof(entrada_t));

	if (!entrada) {
		return entrada;
	}

	entrada->tipo = VACIO;
	strcpy(entrada->nombre, "");

	return entrada;
}

void
llenar_stat_entrada(entrada_t *entrada, struct stat *st)
{
	*st = entrada->st;
}