#!/bin/bash

# Antes de ejecutar las pruebas, en otra terminal montar el fs

cd prueba

prueba_creacion_archivo () {
    echo -e "\n===================================="    
    echo "PRUEBA CREACION ARCHIVO (touch)"
    touch archivo
    if [ -f archivo ];
    then
        echo "> Se creo el archivo correctamente"
    else
        echo "> No se creo el arhivo correctamente"
    fi

    ls
    echo -e "> Se deberia ver el archivo.\n"
    rm archivo
}

prueba_creacion_directorio () {
    echo -e "\n===================================="
    echo "PRUEBA CREACION DIRECTORIO (mkdir)"
    mkdir directorio
    if [ -d directorio ];
    then
        echo "> Se creo el directorio correctamente"
    else
        echo "> No se creo el directorio correctamente"
    fi

    ls
    echo -e "> Se deberia ver el directorio.\n"
    rmdir directorio
}

prueba_borrado_directorio () {
    echo -e "\n===================================="
    echo "PRUEBA BORRADO DIRECTORIO (rmdir)"
    mkdir directorio
    echo "> Se crea el directorio"
    ls
    echo "> Se deberia ver el directorio"
    rmdir directorio
    echo "> Se borra el directorio"
    ls
    echo -e "> No se deberia ver el directorio\n"
}

prueba_borrado_archivo () {
    echo -e "\n===================================="
    echo "PRUEBA BORRADO ARCHIVO (rm)"
    touch archivo
    echo "> Se crea el archivo"
    ls
    echo "> Se deberia ver el archivo"
    rm archivo
    echo "> Se borra el archivo"
    ls
    echo -e "> No se deberia ver el archivo\n"
}

prueba_creacion_directorio_2do_nvl () {
    echo -e "\n===================================="
    echo "PRUEBA CREACION DIRECTORIO 2DO NIVEL"
    mkdir dir1
    echo "> Se creo el directorio de 1er nivel"
    mkdir dir1/dir2
    if [ -d dir1/dir2 ];
    then
        echo "> Se crea un directorio de 2do nivel correctamente"
    else
        echo "> No se creo el directorio correctamente de 2do nivel correctamente"
    fi

    ls dir1
    echo -e "> Se deberia ver el directorio 2.\n"
    rmdir dir1/dir2
    rmdir dir1
}

prueba_borrado_directorio_2do_nvl () {
    echo -e "\n===================================="
    echo "PRUEBA BORRADO DIRECTORIO 2DO NIVEL"
    mkdir dir1
    echo "> Se creo el directorio de 1er nivel"
    mkdir dir1/dir2
    echo "> Se crea un directorio de 2do nivel"
    ls dir1
    echo "> Se deberia ver el directorio 2."
    rmdir dir1/dir2
    echo "> Se borra el directorio de 2do nivel"
    ls dir1
    echo -e "> No se deberia ver el directorio de 2do nivel\n"
    
    rmdir dir1
}

prueba_borrar_directorio_con_contenido () {
    echo -e "\n===================================="
    echo "PRUEBA BORRADO DIRECTORIO CON CONTENIDO"
    mkdir dir1
    echo "> Se creo el directorio de 1er nivel"
    mkdir dir1/dir2
    echo "> Se crea un directorio de 2do nivel"
    echo "> Se intenta borrar el directorio de 1er nivel"
    rmdir dir1
    echo -e "> No se deberia poder borrar un directorio con contenido adentro\n"
    rmdir dir1/dir2
    rmdir dir1
}

prueba_stats () {
    echo -e "\n===================================="
    echo "PRUEBA STATS (stat)"
    touch archivo
    seq 25 > archivo
    echo "Le agrego contenido al archivo"
    stat archivo
    echo -e "> Se muestran las estadisticas del archivo\n"
    rm archivo
}

prueba_stats_access_mod () {
    echo -e "\n===================================="
    echo "PRUEBA STATS ACCESO Y MODIFICACION (stat)"       
    sleep 5
    touch archivo
    stat archivo
    echo "> Se muestran las estadisticas del archivo"
    echo -e "> Vemos que las fechas de acceso y modificacion se actualizaron 5s mas tarde\n"
    rm archivo
}

prueba_lectura_archivos () {
    echo -e "\n===================================="
    echo "PRUEBA LECTURA ARCHIVOS (cat)"
    echo "Hola como estas" > archivo
    cat archivo
    echo -e "> Se lee el contenido del archivo"
    rm archivo
}

prueba_lectura_directorios () {
    echo -e "\n===================================="
    echo "PRUEBA LECTURA DIRECTORIOS (ls)"
    mkdir directorio
    touch archivo
    echo "> Se crean un archivo y un directorio"
    ls
    echo -e "> Se deberian leer un archivo y un directorio\n"
    rm archivo
    rmdir directorio
}

prueba_redireccion_escritura () {
    echo -e "\n===================================="
    echo "PRUEBA REDIRECCION ESCRITURA"
    touch archivo1
    stat archivo1 > archivo2
    echo "> Se escribieron las stats del archivo1 en un archivo2"
    cat archivo2
    echo -e "> Se muestran las estadisticas del archivo1 desde el archivo2\n"
    rm archivo1
    rm archivo2

}

prueba_sobrescribir_archivo () {
    echo -e "\n===================================="
    echo "PRUEBA SOBRESCRIBIR ARCHIVO"
    echo "Texto inicial en archivo" > a
    cat a
    echo "> Se muestra el texto inicial del archivo"
    echo "Texto sobreescrito en archivo" > a
    cat a
    echo -e "> Se muestra el texto sobreescrito en el archivo\n"
    rm a
}

prueba_append_archivo () {
    echo -e "\n===================================="
    echo "PRUEBA APPEND ARCHIVO"
    echo "Texto inicial en archivo. " > a
    cat a
    echo "> Se muestra el texto inicial del archivo"
    echo "Texto escrito a continuacion en archivo" >> a
    cat a
    echo -e "> Se muestra el texto inicial junto al escrito a continuacion en el archivo\n"
    rm a
}

prueba_persistencia () {
    echo -e "\n===================================="
    echo "PRUEBA PERSISTENCIA"
    ls
    echo "> Se deberia ver guardado un archivo_persistencia y un directorio_persistencia"
    ls directorio_persistencia
    echo "> Dentro del directorio_persistencia deberia verse un archivo2"
    echo "> NOTA: Estos archivos solo se genera luego de la primera ejecucion de las pruebas"
    echo -e "> Para ver el resultado optimo, montar nuevamente el filesystem y correr por segunda vez las pruebas\n"
    rm archivo_persistencia
    rm directorio_persistencia/archivo2
    rmdir directorio_persistencia
}

prueba_persistencia
prueba_creacion_archivo
prueba_creacion_directorio
prueba_borrado_directorio
prueba_borrado_archivo
prueba_creacion_directorio_2do_nvl
prueba_borrado_directorio_2do_nvl
prueba_borrar_directorio_con_contenido
prueba_stats
prueba_stats_access_mod
prueba_lectura_archivos
prueba_lectura_directorios
prueba_redireccion_escritura
prueba_sobrescribir_archivo
prueba_append_archivo

touch archivo_persistencia
mkdir directorio_persistencia
touch directorio_persistencia/archivo2

cd ..
umount prueba