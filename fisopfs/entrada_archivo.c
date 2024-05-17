#include "entrada_archivo.h"


int
crear_archivo(entrada_archivo_t *archivo, char *nombre, mode_t modo)
{
	archivo->tipo = ARCHIVO;
	archivo->st.st_size = 0;
	archivo->st.st_uid = getuid();
	archivo->st.st_gid = getgid();
	archivo->st.st_mode = modo;
	archivo->st.st_mtime = time(NULL);
	archivo->st.st_atime = time(NULL);
	archivo->st.st_ctime = time(NULL);
	archivo->st.st_nlink = 1;
	archivo->st.st_ino = 0;
	archivo->st.st_blocks = 0;
	archivo->st.st_blksize = 0;

	if (strlen(nombre) < MAXIMO_PATH) {
		strcpy(archivo->nombre, nombre);
	} else {
		return -EINVAL;
	}

	archivo->contenido = NULL;

	return 0;
}


void
liberar_archivo(entrada_archivo_t *archivo)
{
	if (!archivo) {
		return;
	}

	if (archivo->contenido) {
		free(archivo->contenido);
	}

	free(archivo);
}


entrada_archivo_t *
crear_entrada_archivo_desde_archivo(FILE *archivo_a_leer)
{
	entrada_archivo_t *archivo = malloc(sizeof(entrada_archivo_t));

	if (!archivo) {
		return NULL;
	}

	int resultado =
	        fread(archivo, sizeof(entrada_archivo_t), 1, archivo_a_leer);

	if (resultado < 1) {
		free(archivo);
		return NULL;
	}

	if (archivo->st.st_size != 0) {
		// reserva espacio para el contenido
		archivo->contenido = malloc(archivo->st.st_size * sizeof(char));
		if (!archivo->contenido) {
			free(archivo);
			return NULL;
		}
		// lee el contenido
		resultado = fread(archivo->contenido,
		                  sizeof(char) * archivo->st.st_size,
		                  1,
		                  archivo_a_leer);
		if (resultado < 1) {
			liberar_archivo(archivo);
			return NULL;
		}
	} else {
		archivo->contenido = NULL;
	}

	return archivo;
}


int
escribir_entrada_archivo_en_archivo(entrada_archivo_t *archivo,
                                    FILE *archivo_a_escribir)
{
	int resultado =
	        fwrite(archivo, sizeof(entrada_archivo_t), 1, archivo_a_escribir);

	if (resultado != 1) {
		return -EIO;
	}
	// se escrive el contendio
	fwrite(archivo->contenido,
	       sizeof(char) * archivo->st.st_size,
	       1,
	       archivo_a_escribir);

	if (resultado != 1) {
		return -EIO;
	}

	return 0;
}