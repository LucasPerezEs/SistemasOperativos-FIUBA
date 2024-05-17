#include "entrada_directorio.h"

// Completa las entradas con entradas_t
int
llenar_directorio_con_entradas_vacias(entrada_directorio_t *directorio)
{
	for (size_t j = directorio->cantidad_entradas_ocupadas;
	     j < directorio->cantidad_entradas;
	     j++) {
		directorio->entradas[j] = crear_entrada_vacia();

		if (!directorio->entradas[j]) {
			return -ENOMEM;
		}
	}

	return 0;
}


int
expandir_entrdas_dir(entrada_directorio_t *directorio)
{
	if (directorio->cantidad_entradas ==
	    directorio->cantidad_entradas_ocupadas) {
		// si pasa esto, es necesario expandir la cantidad de
		// entradas
		if (directorio->cantidad_entradas == 0) {
			directorio->cantidad_entradas = ENTRADAS_INICIALES;

			directorio->entradas =
			        malloc(directorio->cantidad_entradas *
			               sizeof(entrada_t *));

		} else {
			// se multiplica por dos la cantidad de entradas
			directorio->cantidad_entradas *= MULTIPLICADOR_ENTRADAS;

			directorio->entradas =
			        realloc(directorio->entradas,
			                directorio->cantidad_entradas *
			                        sizeof(entrada_t *));
		}

		if (!directorio->entradas) {
			return -ENOMEM;
		}

		int resultado = llenar_directorio_con_entradas_vacias(directorio);
		if (resultado != 0) {
			return resultado;
		}
	}

	return 0;
}


int
colocar_entrada_dentro_de_directorio(entrada_directorio_t *directorio,
                                     entrada_t *nueva_entrada)
{
	if (!nueva_entrada) {
		return -EFAULT;
	}

	entrada_t *entrada;
	// busca en el directorio una entrada vacia
	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada = directorio->entradas[i];
		if (entrada->tipo == VACIO) {
			free(entrada);

			directorio->entradas[i] = nueva_entrada;

			// actualiza el dir y de ser necesario lo expande
			directorio->cantidad_entradas_ocupadas++;
			directorio->st.st_mtime = time(NULL);
			directorio->st.st_atime = time(NULL);
			directorio->st.st_ctime = time(NULL);


			int resultado = expandir_entrdas_dir(directorio);
			if (resultado != 0) {
				return resultado;
			}

			return 0;
		}
	}

	return -ENOENT;
}


int
crear_dir(entrada_directorio_t *directorio, const char *path, mode_t mode)
{
	if (!directorio) {
		return -EFAULT;
	}

	directorio->tipo = DIRECTORIO;
	directorio->st.st_size = 0;
	directorio->st.st_uid = getuid();
	directorio->st.st_gid = getgid();
	directorio->st.st_mode = mode;
	directorio->st.st_mtime = time(NULL);
	directorio->st.st_atime = time(NULL);
	directorio->st.st_ctime = time(NULL);

	directorio->st.st_nlink = 1;
	directorio->st.st_ino = 0;
	directorio->st.st_blocks = 0;
	directorio->st.st_blksize = 0;

	if (strlen(path) < MAXIMO_PATH) {
		strcpy(directorio->nombre, path);
	} else {
		return -EINVAL;
	}

	directorio->cantidad_entradas = 0;
	directorio->cantidad_entradas_ocupadas = 0;

	int resultado = expandir_entrdas_dir(directorio);

	if (resultado != 0) {
		return resultado;
	}

	return 0;
}


void
liberar_dir(entrada_directorio_t *directorio)
{
	if (!directorio) {
		return;
	}
	entrada_t *entrada;
	entrada_directorio_t *entrada_dir;
	entrada_archivo_t *entrada_archivo;

	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada = directorio->entradas[i];

		if (entrada) {
			if (entrada->tipo == DIRECTORIO) {
				entrada_dir = (entrada_directorio_t *) entrada;
				liberar_dir(entrada_dir);
			} else if (entrada->tipo == ARCHIVO) {
				entrada_archivo = (entrada_archivo_t *) entrada;
				liberar_archivo(entrada_archivo);
			} else {
				free(directorio->entradas[i]);
			}
		}
	}

	if (directorio->entradas) {
		free(directorio->entradas);
	}

	free(directorio);
}

entrada_directorio_t *
crear_directorio_desde_archivo(FILE *archivo)
{
	entrada_directorio_t *directorio = malloc(sizeof(entrada_directorio_t));
	if (!directorio) {
		return NULL;
	}

	// lee le directorio desde archivo
	size_t resultado =
	        fread(directorio, sizeof(entrada_directorio_t), 1, archivo);


	if (resultado < 1) {
		free(directorio);
		return NULL;
	}

	// reserva espacio para entradas
	directorio->entradas =
	        malloc(directorio->cantidad_entradas * sizeof(entrada_t *));

	if (!directorio->entradas) {
		free(directorio);
		return NULL;
	}

	for (int j = 0; j < directorio->cantidad_entradas_ocupadas; j++) {
		entrada_t entrada;

		resultado = fread(&entrada, sizeof(entrada_t), 1, archivo);

		if (resultado < 1) {
			liberar_dir(directorio);
			return NULL;
		}

		switch (entrada.tipo) {
		case DIRECTORIO:
			fseek(archivo,
			      -sizeof(entrada_t),
			      SEEK_CUR);  // Retroceder para leer la struct completa

			entrada_directorio_t *sub_directorio =
			        crear_directorio_desde_archivo(archivo);

			if (!sub_directorio) {
				liberar_dir(directorio);
				return NULL;
			}

			directorio->entradas[j] = (entrada_t *) sub_directorio;

			break;
		case ARCHIVO:
			fseek(archivo,
			      -sizeof(entrada_t),
			      SEEK_CUR);  // Retroceder para leer la struct completa

			directorio->entradas[j] =
			        (entrada_t *) crear_entrada_archivo_desde_archivo(
			                archivo);

			break;
		default:
			return NULL;
		}

		if (!directorio->entradas[j]) {
			return NULL;
		}
	}

	resultado = llenar_directorio_con_entradas_vacias(directorio);
	if (resultado != 0) {
		return NULL;
	}

	return directorio;
}

int
escribir_directorio_en_archivo(entrada_directorio_t *directorio, FILE *archivo)
{
	int resultado =
	        fwrite(directorio, sizeof(entrada_directorio_t), 1, archivo);

	if (resultado != 1) {
		return -EIO;
	}

	for (int j = 0; j < directorio->cantidad_entradas; j++) {
		entrada_t *entrada = directorio->entradas[j];

		switch (entrada->tipo) {
		case DIRECTORIO:
			resultado = escribir_directorio_en_archivo(
			        (entrada_directorio_t *) entrada, archivo);

			if (resultado != 0) {
				return resultado;
			}

			break;
		case ARCHIVO:
			resultado = escribir_entrada_archivo_en_archivo(
			        (entrada_archivo_t *) entrada, archivo);

			if (resultado != 0) {
				return resultado;
			}

			break;
		case VACIO:
			break;
		default:
			return -EIO;
		}
	}

	return 0;
}


bool
esta_vacio_directorio(entrada_directorio_t *directorio)
{
	return directorio->cantidad_entradas_ocupadas == 0;
}