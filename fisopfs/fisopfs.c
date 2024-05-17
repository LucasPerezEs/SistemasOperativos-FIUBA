#include "fisopfs.h"

long long unsigned MAX_ESPACIO_FS = 0ULL;
char *ARCHIVO_PERSISTENCIA = "";

fisopfs_fs_t fs;

void
crear_directorio_raiz()
{
	fs.raiz = malloc(sizeof(entrada_directorio_t));

	int resultado = crear_dir(fs.raiz, "/", __S_IFDIR | 0755);

	if (resultado != 0) {
		perror("Error al crear directorio raiz");
		exit(EXIT_FAILURE);
	}

	fs.raiz->st.st_uid = 1717;
	// iniciliza la cantida de espacio ocupado por el file system
	fs.espacio_total_ocupado_fs =
	        sizeof(fisopfs_fs_t) + sizeof(entrada_directorio_t);
}


entrada_t *
encontrar_entrada_rec(char *entrada_a_,
                      const char *resto_del_path,
                      entrada_directorio_t *directorio)
{
	// actualiza el ultimo acceso del directorio
	directorio->st.st_atime = time(NULL);
	entrada_t *entrada;

	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada = directorio->entradas[i];

		if (entrada->tipo != VACIO &&
		    strcmp(entrada->nombre, entrada_a_) == 0) {
			if (resto_del_path && entrada->tipo == DIRECTORIO) {
				// si hay resto de path y es un directorio la
				// entrada actual, quedan elementos por buscar,
				// se sigue la recursividad
				char siguiente_nombre_entrada_a_[MAXIMO_PATH];
				const char *siguiente_resto_del_path;

				dividir_path_en_dos(resto_del_path,
				                    siguiente_nombre_entrada_a_,
				                    &siguiente_resto_del_path);

				return encontrar_entrada_rec(
				        siguiente_nombre_entrada_a_,
				        siguiente_resto_del_path,
				        (entrada_directorio_t *) entrada);
			} else if (resto_del_path && entrada->tipo != DIRECTORIO) {
				// si hay resto del path pero no es un
				// directorio la entrada la ruta es invalida
				return NULL;
			} else {
				// si no hay resto de path se termino la busquda
				return entrada;
			}
		}
	}
	return NULL;
}


entrada_t *
encontrar_entrada(const char *path)
{
	if (strcmp(path, "/") == 0) {
		return (entrada_t *) fs.raiz;
	}

	char entrada_a_[MAXIMO_PATH];
	const char *resto_del_path;
	dividir_path_en_dos(path + 1, entrada_a_, &resto_del_path);

	return encontrar_entrada_rec(entrada_a_, resto_del_path, fs.raiz);
}


static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}


	entrada_t *entrada = encontrar_entrada(path);

	if (!entrada) {
		return -ENOENT;
	}

	llenar_stat_entrada(entrada, st);

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// se busca el directorio
	entrada_directorio_t *directorio =
	        (entrada_directorio_t *) encontrar_entrada(path);

	if (directorio->tipo != DIRECTORIO) {
		return -ENOENT;
	}


	entrada_t *entrada;

	// se lee las entradas del directorio
	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada = directorio->entradas[i];
		if (entrada->tipo != VACIO) {
			filler(buffer, entrada->nombre, NULL, 0);
		}
	}

	// actuliza el dir
	directorio->st.st_atime = time(NULL);
	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}


	entrada_archivo_t *archivo =
	        (entrada_archivo_t *) encontrar_entrada(path);

	if (!archivo || archivo->tipo != ARCHIVO) {
		return -ENOENT;
	}

	if (offset + size > archivo->st.st_size)
		size = archivo->st.st_size - offset;

	size = size > 0 ? size : 0;

	memcpy(buffer, archivo->contenido + offset, size);

	// actuliza el archivo
	archivo->st.st_atime = time(NULL);
	return size;
}


void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisopfs_init - proto_major: %i, proto minor: %i, "
	       "async_read: %i, max_write: %i, max_readahead: %i, capable: %i, "
	       "want: % i, max_background: % i, congestion_threshold: % i \n",
	       conn->proto_major,
	       conn->proto_minor,
	       conn->async_read,
	       conn->max_write,
	       conn->max_readahead,
	       conn->capable,
	       conn->want,
	       conn->max_background,
	       conn->congestion_threshold);

	FILE *archivo_fs = fopen(ARCHIVO_PERSISTENCIA, "rb");

	if (!archivo_fs) {
		crear_directorio_raiz();

	} else {
		size_t resultado =
		        fread(&fs, sizeof(fisopfs_fs_t), 1, archivo_fs);

		if (resultado != 1) {
			perror("Fallo en la lectura del archivo");
			exit(EXIT_FAILURE);
		}

		fs.raiz = crear_directorio_desde_archivo(archivo_fs);

		if (!fs.raiz) {
			perror("Fallo en la lectura del archivo");
			exit(EXIT_FAILURE);
		}

		fclose(archivo_fs);
	}

	return NULL;
}


static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug]fisop_mkdir - path: %s, mode: %u\n", path, mode);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	// verifica si hay espacio en el file system para mas memoria
	if (fs.espacio_total_ocupado_fs + sizeof(entrada_directorio_t) >
	    MAX_ESPACIO_FS) {
		return -ENOSPC;
	}

	// obtiene el nombre del directorio
	char path_directorio[MAXIMO_PATH];
	obtener_directorio_path(path, path_directorio);

	// separa el nombre del nuevo subdir
	char nombre_nuevo_dir[MAXIMO_PATH];
	obtener_nombre_final_path(path, nombre_nuevo_dir);

	// busca el directorio que va a contener al nuevo subdir
	entrada_directorio_t *directorio =
	        (entrada_directorio_t *) encontrar_entrada(path_directorio);

	if (directorio->tipo != DIRECTORIO) {
		return -ENOENT;
	}

	// reserva espacio para el nuevo dir
	entrada_directorio_t *nuevo_directorio =
	        malloc(sizeof(entrada_directorio_t));

	// setea el dir
	int resultado =
	        crear_dir(nuevo_directorio, nombre_nuevo_dir, __S_IFDIR | mode);

	if (resultado != 0) {
		free(nuevo_directorio);
		return resultado;
	}

	// coloca el nuevo directorio dentro del directorio
	resultado = colocar_entrada_dentro_de_directorio(
	        directorio, (entrada_t *) nuevo_directorio);

	if (resultado != 0) {
		free(nuevo_directorio);
		return resultado;
	}

	// actualiza el fs
	fs.espacio_total_ocupado_fs += sizeof(entrada_directorio_t);
	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug]fisop_unlink - path : %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	char path_directorio[MAXIMO_PATH];
	obtener_directorio_path(path, path_directorio);

	char nombre_archivo[MAXIMO_PATH];
	obtener_nombre_final_path(path, nombre_archivo);

	entrada_directorio_t *directorio =
	        (entrada_directorio_t *) encontrar_entrada(path_directorio);

	if (directorio->tipo != DIRECTORIO) {
		return -ENOENT;
	}

	// busca la entrada a eliminar en el directorio
	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada_t *entrada = directorio->entradas[i];

		if (entrada->tipo == ARCHIVO &&
		    strcmp(entrada->nombre, nombre_archivo) == 0) {
			// se crea una nueva entrada vacia
			entrada_t *nueva_entra_vacia;

			if (!(nueva_entra_vacia = crear_entrada_vacia())) {
				return -ENOMEM;
			}

			// se borra el directorio
			liberar_archivo((entrada_archivo_t *) entrada);

			directorio->entradas[i] = nueva_entra_vacia;

			// actualiza el directorio
			directorio->st.st_mtime = time(NULL);
			directorio->st.st_atime = time(NULL);
			directorio->st.st_ctime = time(NULL);
			directorio->cantidad_entradas_ocupadas--;

			// actualiza el fs
			fs.espacio_total_ocupado_fs -= sizeof(entrada_archivo_t);

			return 0;
		}
	}

	return ENOENT;
}


static int
fisopfs_rmdir(const char *path)
{
	printf("[debug]fisop_rmdir - path: %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	char path_directorio[MAXIMO_PATH];
	obtener_directorio_path(path, path_directorio);

	char nombre_dir_a_borar[MAXIMO_PATH];
	obtener_nombre_final_path(path, nombre_dir_a_borar);

	entrada_directorio_t *directorio =
	        (entrada_directorio_t *) encontrar_entrada(path_directorio);

	if (directorio->tipo != DIRECTORIO) {
		return -ENOENT;
	}

	entrada_t *entrada;
	// busca el subdir a eliminar del del directorio
	for (size_t i = 0; i < directorio->cantidad_entradas; i++) {
		entrada = directorio->entradas[i];
		if (entrada->tipo == DIRECTORIO &&
		    strcmp(entrada->nombre, nombre_dir_a_borar) == 0) {
			// verifica que el directori este vacio
			if (!esta_vacio_directorio(
			            (entrada_directorio_t *) entrada)) {
				return -EPERM;
			}

			// se crea una nueva entrada vacia
			entrada_t *nueva_entra_vacia;

			if (!(nueva_entra_vacia = crear_entrada_vacia())) {
				return -ENOMEM;
			}

			// se borra el directorio
			liberar_dir((entrada_directorio_t *) entrada);

			directorio->entradas[i] = nueva_entra_vacia;

			// actualiza el directorio
			directorio->st.st_mtime = time(NULL);
			directorio->st.st_atime = time(NULL);
			directorio->st.st_ctime = time(NULL);
			directorio->cantidad_entradas_ocupadas--;

			// actualiza el fs
			fs.espacio_total_ocupado_fs -=
			        sizeof(entrada_directorio_t);
			return 0;
		}
	}

	return ENOENT;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug]fisop_truncate - path: %s, size: %li\n", path, size);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	entrada_archivo_t *entrada =
	        (entrada_archivo_t *) encontrar_entrada(path);

	if (entrada && entrada->tipo == DIRECTORIO) {
		return -EISDIR;
	} else if (!entrada || entrada->tipo != ARCHIVO) {
		return -ENOENT;
	}

	if (size != 0) {
		// verifica si hay espacio en el file system para mas memoria
		if (fs.espacio_total_ocupado_fs + size - entrada->st.st_size >
		    MAX_ESPACIO_FS) {
			return -ENOSPC;
		}

		// reserva el espacio pediodo
		char *nuevo_contenido =
		        realloc(entrada->contenido, size * sizeof(char));

		if (!nuevo_contenido) {
			return -ENOMEM;
		}
		entrada->contenido = nuevo_contenido;
	}

	// actualiza el tamañio del sistema de archivos
	fs.espacio_total_ocupado_fs -= entrada->st.st_size;
	fs.espacio_total_ocupado_fs += size;

	// actuliza el archivo
	entrada->st.st_size = size;
	entrada->st.st_atime = time(NULL);
	entrada->st.st_mtime = time(NULL);
	entrada->st.st_ctime = time(NULL);

	return 0;
}


int
fisopfs_utimens(const char *path, const struct timespec ts[2])
{
	printf("[debug]fisop_utimes - path: %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	entrada_t *entrada = encontrar_entrada(path);

	if (!entrada) {
		return ENOENT;
	}

	entrada->st.st_atime = ts[0].tv_sec;
	entrada->st.st_mtime = ts[0].tv_sec;
	return 0;
}


int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug]fisop_create - path: %s, mode: %u\n", path, mode);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}
	// verifica si hay espacio en el file system para mas memoria
	if (fs.espacio_total_ocupado_fs + sizeof(entrada_archivo_t) >
	    MAX_ESPACIO_FS) {
		return -ENOSPC;
	}

	char path_directorio[MAXIMO_PATH];
	obtener_directorio_path(path, path_directorio);

	char nombre_archivo[MAXIMO_PATH];
	obtener_nombre_final_path(path, nombre_archivo);

	entrada_directorio_t *directorio =
	        (entrada_directorio_t *) encontrar_entrada(path_directorio);

	if (directorio->tipo != DIRECTORIO) {
		return -ENOENT;
	}

	// verifica si es un archivo
	if (S_ISREG(mode)) {
		// crea el nuevo archivo
		entrada_archivo_t *nuevo_archivo =
		        malloc(sizeof(entrada_archivo_t));

		if (!nuevo_archivo) {
			return -ENOMEM;
		}

		int resultado =
		        crear_archivo(nuevo_archivo, nombre_archivo, mode);

		if (resultado != 0) {
			free(nuevo_archivo);
			return resultado;
		}

		// lo coloca en el directorio
		resultado = colocar_entrada_dentro_de_directorio(
		        directorio, (entrada_t *) nuevo_archivo);

		if (resultado != 0) {
			free(nuevo_archivo);
			return resultado;
		}

		// actualiza el espacio ocupado por el file system
		fs.espacio_total_ocupado_fs += sizeof(entrada_archivo_t);
		return 0;
	}

	return ENOENT;
}


int
fisopfs_write(const char *path,
              const char *buf,
              unsigned long size,
              long offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, buf: %s, offset: %lu, size: "
	       "%lu\n",
	       path,
	       buf,
	       offset,
	       size);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	entrada_archivo_t *archivo =
	        (entrada_archivo_t *) encontrar_entrada(path);

	// verifica el archivo
	if (archivo && archivo->tipo == DIRECTORIO) {
		return -EISDIR;
	} else if (!archivo || archivo->tipo != ARCHIVO) {
		return -ENOENT;
	}

	// verifica si es expandir el contenido archivo
	if (offset + size > archivo->st.st_size) {
		// verifica si hay espacio en el file system para mas memoria
		if (fs.espacio_total_ocupado_fs + size + offset -
		            archivo->st.st_size >
		    MAX_ESPACIO_FS) {
			return -ENOSPC;
		}

		char *nuevo_contenido = realloc(archivo->contenido,
		                                (size + offset) * sizeof(char));

		if (!nuevo_contenido) {
			return -EIO;
		}

		archivo->contenido = nuevo_contenido;

		// actualiza el tamañio del sistema de archivos
		fs.espacio_total_ocupado_fs -= archivo->st.st_size;
		fs.espacio_total_ocupado_fs += size + offset;

		// actualiza el archivo
		archivo->st.st_size = offset + size;
	}

	size = size > 0 ? size : 0;
	memcpy(archivo->contenido + offset, buf, size);

	// se actuliza el archivo
	archivo->st.st_mtime = time(NULL);
	archivo->st.st_atime = time(NULL);
	archivo->st.st_ctime = time(NULL);

	return size;
}


int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush - path: %s\n", path);

	// verifica el path
	if (strlen(path) > MAXIMO_PATH) {
		return -EINVAL;
	}

	FILE *archivo = fopen(ARCHIVO_PERSISTENCIA, "wb");

	if (!archivo) {
		perror("Error al abrir el archivo");
		return -1;
	}
	// escribe la estructura fs
	int resultado = fwrite(&fs, sizeof(fisopfs_fs_t), 1, archivo);

	if (resultado != 1) {
		fclose(archivo);
		return -EIO;
	}

	// escribe el resto del fs
	resultado = escribir_directorio_en_archivo(fs.raiz, archivo);

	// verifica el resultado de escribir el resto del fs
	if (resultado != 0) {
		fclose(archivo);
		return resultado;
	}

	fclose(archivo);
	return 0;
}


void
fisopfs_destroy(void *private_data)
{
	printf("[debug]fisop_destroy\n");

	fisopfs_flush("/", NULL);

	liberar_dir(fs.raiz);
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.init = fisopfs_init,
	.destroy = fisopfs_destroy,
	.mkdir = fisopfs_mkdir,
	.unlink = fisopfs_unlink,
	.rmdir = fisopfs_rmdir,
	.truncate = fisopfs_truncate,
	.utimens = fisopfs_utimens,
	.write = fisopfs_write,
	.flush = fisopfs_flush,
	.create = fisopfs_create,
};

void
parse_args(int *argc, char *argv[])
{
	// contador_args tiene que ver con la convencion de fuse, que pide una determinada cantidad de argc's
	int contador_args = 0;
	for (int i = 0; i < *argc; i++) {
		if (strcmp("--archivo-persistencia", argv[i]) == 0) {
			if (argv[i + 1] == NULL) {
				fprintf(stderr, "Error: no se especifico el path del archivo de persistencia\n");
				exit(EXIT_FAILURE);
			}
			ARCHIVO_PERSISTENCIA = argv[i + 1];
			argv[i] = NULL;
			argv[i + 1] = NULL;
			contador_args++;
			i++;
			continue;
		}
		if (strcmp("--max_espacio_fs", argv[i]) == 0) {
			if (argv[i + 1] == NULL) {
				fprintf(stderr, "Error: no se especifico el maximo espacio del file system\n");
				exit(EXIT_FAILURE);
			}
			MAX_ESPACIO_FS = strtoull(
			        argv[i + 1],
			        NULL,
			        10);  // strtoull convierte a unsigned long long int
			argv[i] = NULL;
			argv[i + 1] = NULL;
			contador_args++;
			i++;
		}
	}
	*argc -= contador_args * 2;
	if (MAX_ESPACIO_FS == 0) {
		MAX_ESPACIO_FS = 4294967296ULL;
	}
	if (strcmp(ARCHIVO_PERSISTENCIA, "") == 0)
		ARCHIVO_PERSISTENCIA = ARCHIVO_PERSISTENCIA_DEFAULT;
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr,
		        "Uso: %s <flags FUSE> <punto de montaje> (optional) -> "
		        "[--archivo-persistencia] <path archivo "
		        "persistencia>\n [--max_espacio_fs] <maximo espacio>",
		        argv[0]);
		return 1;
	}

	parse_args(&argc, argv);
	return fuse_main(argc, argv, &operations, NULL);
}