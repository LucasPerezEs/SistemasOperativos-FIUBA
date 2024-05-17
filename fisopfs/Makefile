CC = gcc
CFLAGS := -ggdb3 -O2 -Wall -std=c11
CFLAGS += -Wno-unused-function -Wvla

# Flags for FUSE
LDLIBS := $(shell pkg-config fuse --cflags --libs)

# Name for the filesystem!
FS_NAME := fisopfs

SRCS = fisopfs.c entrada.c entrada_archivo.c entrada_directorio.c utilidades.c
# entrada.c
all: build
	
build: $(FS_NAME)

# Regla para el ejecutable
$(FS_NAME): 
	$(CC) $(CFLAGS) $(SRCS) -o $(FS_NAME) $(LDLIBS)



format: .clang-files .clang-format
	xargs -r clang-format -i <$<

valgrind: fisopfs
	valgrind --leak-check=full -s ./fisopfs -f prueba/

docker-build:
	docker build -t fisopfs:latest .

docker-run:
	docker container run -it --rm --name=fisopfs \
		-v $(shell pwd):/fisopfs \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		--security-opt apparmor:unconfined \
		fisopfs:latest \
		bash 

docker-attach:
	docker exec -it fisopfs bash

clean:
	rm -rf $(EXEC) *.o core vgcore.* $(FS_NAME)

.PHONY: all build clean format docker-build docker-run docker-attach
