# SisOp TP 3: File System FUSE

- Informe: 
  - Estructura general del File System 
  - Estructuras del FS
    - Archivos
    - Directorios 
    - Entradas 
    - File System 

  - Operacion de busqueda 
  - Persistencia  
  - Limitaciones usuario 
  - Ejecución de pruebas
  - Salida esperada de pruebas
  - Aclaraciones finales 

## Estructura general del trabajo   

Un primer pantallazo general a como se ve el trabajo y la relación de sus estructuras es la siguiente:    

![estructura general fs](https://github.com/fiubatps/sisop_2023b_g21/assets/23730941/c9e44bfa-b578-4409-8e0f-5b7c7632ea9b)

A diferencia de la implementación de otros file system que funcionan a base de inodos, dentry y bloques, el nuestro se parece mucho más a cómo alguien, sin saber nada de como funciona por detrás un  file system, lo pensaría con estructuras que representan archivos y directorios relacionados.  
Como una explicación general del trabajo, podemos decir que nuestra implementación de sistema de archivos es un árbol invertido. La raíz está representada con la estructura `fisopfs_fs_t`, que contiene información general del sistema de archivos y un puntero al directorio raíz. Los directorios tienen un puntero a un vector de entradas dinámico. Este vector de entradas puede albergar archivos, otros directorios o entradas vacías. Esta última estructura representa los espacios libres dentro del directorio. Los archivos por su parte tienen un puntero a un vector de caracteres dinámico donde guardan su contenido. 


## Estructuras del trabajo 
Como ya se contó, nuestro trabajo utiliza 4 estructuras principales: 
- `entrada_archivo_t`
- `entrada_directorio_t`
- `entrada_t`
- `fisopfs_fs_t`
A continuación se detalla un poco cada uno de ellas: 

### entrada_archivo _t

![entrada_archivo](https://github.com/fiubatps/sisop_2023b_g21/assets/23730941/1b8e8ddb-4c67-4e91-a5b8-006e7dac3476)

La representación de un archivo. Cada archivo tiene un nombre, el cual es relativo a la carpeta que se encuentra (`MAX_PATH` fija el tamaño máximo del nombre  dado que, ningún directorio puede contener un archivo con un nombre más largo al máximo path permitido). También contiene un struct stat con metada del mismo: última modificación,  tamaño del contenido, usuario, grupo usuario, etc. Por último, tiene una cadena que representa el tipo de entrada y un puntero al contenido. El contenido como ya se mencionó es dinámico y cambia según las operaciones de truncate y write.


### entrada_directorio_t

![entrada_directorio_t](https://github.com/fiubatps/sisop_2023b_g21/assets/23730941/a45b49d0-60cf-4083-b4ea-b7864529151e)

La representación de un directorio. Los 3 primeros campos son los mismos que los de los archivos. Además, cuenta con 3 campos más: la cantidad de entradas, cantidad de entradas ocupadas y un puntero al vector con con punteros a entradas. Algo que se puede notar en este esquema como en otros, es que el vector de entradas siempre se encuentra inicializado con `entradas_t`, por más que directorio no tenga netamente nada y no albergue un archivo o subdirectorio. Esto se hace con fines prácticos, trayendo importantes ventajas en muchas operaciones (más adelante se ejemplifica más este tema). Por eso se hace la distinción entre cantidad de entradas, que es el tamaño propio del vector y la cantidad de entradas ocupadas, que es la cantidad de entradas realmente ocupadas por un subdirectorio o archivo.  Como ya se mencionó el tamaño del vector es dinámico, permitiendo que los directores puedan tener hasta n entradas. El tamaño de este se reajusta al final de cada `mkdir` y `create`, donde si la cantidad de entradas es igual a la cantidad de entradas ocupadas, se duplica la disponibilidad de este, permitiendo que no exista el caso donde el directorio no tenga espacio.  


### entrada_t


![entrada_t](https://github.com/fiubatps/sisop_2023b_g21/assets/23730941/a9fa10dd-685a-47f5-811a-be1f9deb4bfe)


La representación de una entrada dentro de un directorio. Los 3 primeros campos son los mismos que los de los archivos y directorios. Aunque el nombre es siempre vacío y el tipo siempre es `VACIO`. Que las 3 estructuras compartan los primeros 3 campos no es casual y es lo que nos permite cierta especie de polimorfismo entre estas, y que vuelve  a la entrada parte fundamental de nuestra implementación. La necesidad de tener una estructura que representa a la entrada apareció sobre todo con la precisión de poder tener en un mismo vector directorios y archivos sin distinción. Dándole vueltas a este asunto, nace la entrada, una estructura que define los rasgos comunes entre los archivos y directorios y a su vez, representa los espacios vacíos. La entrada junto con la ayuda de los casteos nos permite facilitar muchísimo las operaciones, sobre todo la de búsqueda. 
Facilitar la operación de búsqueda no es poca cosa, siendo en un sistema de archivos la que más se hace. La existencia de la entrada facilita la búsqueda, primero, como ya se mencionó, porque permite tener directorios y archivos en un mismo  vector, y segundo, porque se borra la necesidad de distinguir entre uno y otro. Si se quiere buscar un archivo o subdirectorio por su nombre, simplemente se recorre el vector de entradas comparando el campo de nombre con el que se busca. Si se quiere buscar una entrada vacía dentro de un directorio, busco dentro de las entradas del directorio aquella que sea de tipo vacía. Y si una vez que encuentro la entrada que busco, necesito hacer algo específico de un directorio o archivo, se castea.
Otras operaciones que facilita bastante la entrada es la lectura,  al momento de levantar el sistema de archivos desde disco(se aborada esto mas adelante).     


### fisopfs_fs_t

![fisopfs_fs_t](https://github.com/fiubatps/sisop_2023b_g21/assets/23730941/2f265c66-12a9-4792-a936-7382335d3c20)


La estructura base del sistema de archivos, cuya idea principal es el almacenar  metadatos acerca del mismo. Cuenta con dos campos: el primero es la cuenta del actualizada del espacio total  que está ocupando el sistema de archivos, siendo  esta estructura la encargada de llevar a cabo dicha cuenta (más adelante profundizamos en esto), y un puntero al directorio raíz.
Esta estructura es la que se inicializa siempre al iniciar el sistema de archivos y se mantiene como variable global. 

## Operacion de busqueda 
La operación de búsqueda es una de las cosas que más hace en un sistema de archivos. Por cada operación que se hace, FUSE nos da el path absoluto con respecto al punto de montaje y es nuestro trabajo a partir de ese path encontrar el archivo o directorio correspondiente. En sí, la forma planteada para la búsqueda no es muy complicada (quizás la mayor dificultad a la que nos enfrentamos es el manejo de strings en C). Semejante a como actúa un sistema de archivos real, para la búsqueda lo que se  hace es ir descomponiendo el path por elementos (tokens) marcados por `/`, empezando la búsqueda desde el directorio raíz.  Para explicar mejor damos un ejemplo: 

Supongamos que llega el path: `/trabajos/sisop/archivesco`. Lo primero que se hace es verificar si el directorio es el raíz(`/`). En caso de serlo, se devuelve el directorio raíz almacenado en `fisopfs_fs_t`. En caso de no serlo, se elimina el primer carácter y comienza la búsqueda dentro del directorio raíz. A partir de acá de forma recursiva, se va ir dividiendo el path por la primera ocurrencia de `/`. La primera vez que se haga en este caso se va a dividir en `trabajos` por un lado y `sisop/archivesco` por otro. Como las estructuras de nuestro sistema de archivos almacenan sus nombres, lo que se va hacer es comparar en el directorio raíz si tiene alguna entrada llamada `trabajos`. Si no se encuentra entrada que coincida o se encuentra una entrada que coincida pero no es un directorio se termina la búsqueda. Si se encuentra y es de tipo de directorio se continúa la búsqueda recursiva. Supongamos que estamos en el caso de este último, a continuación se va a dividir `sisop/archivesco` en `sisop` por un lado y `archivesco` por el otro y se va repetir la misma lógica, buscando la entrada `archivesco` en el directorio `sisop`. Supongamos que se encuentra, como siguiente paso se va intentar separar `archivesco` en dos, pero como no se puede , va quedar por un lado solamente archivesco (y `NULL` por el otro lado). De esta manera, al encontrar una entrada que coincida con `archivesco`  se sabe que hay que terminar la búsqueda porque no hay más path para buscar. Esta es la manera en que encontramos directorio y archivos en nuestro sistema de archivos. 

## Persistencia 


La persistencia en disco es lo que permite que nuestro sistema de archivos mantenga sus datos y metadatos al desmontar y montar nuevamente. Al desmontar, se persiste toda la información necesaria para poder volver a reconstruirlo en un archivo con extensión `.fisopfs`. El usuario tiene la posibilidad con el flag `--archivo-persistencia` de establecer de cuál archivo se va a leer y escribir al montar y desmontar el fs. En caso de no designarse se hará en el archivo `persistencia.fisopfs` de forma default.

La persistencia podríamos decir que consiste en dos procesos: uno de escritura(que ocurre en flush y en destroy) y otro de lectura (en init). Empecemos detallando el primero. 
Para la escritura, vamos recorriendo de forma recursiva todo nuestro sistema de archivos desde la raíz escribiendo el contenido en un archivo en disco.  Siempre se empieza escribiendo la data de la estructura `fisopfs_fs_t` , para luego iniciar el proceso recursivo de escritura desde el directorio raíz.  Estos niveles de recursividad consiste primero en escribir el directorio para luego escribir las entradas que alberga. Si la entrada es un archivo escribimos primero la metadata que este contiene en la estructura y luego su contenido. Si la entrada está vacía no la escribimos. Esta es una importante decisión de diseño que tomamos y como la existencia de las entradas vacías solo tiene fines implementativos y no guarda información importante, no es necesario escribirlas y que el usuario pague el costo de almacenarlas. Si la entrada es un directorio se repite esta serie de pasos. 
A continuación se deja un diagrama para explicar mejor. 

![image](https://github.com/fiubatps/sisop_2023b_g21/assets/127911304/bdc77827-6d56-4937-811c-6cb659f1b8a6)
![image](https://github.com/fiubatps/sisop_2023b_g21/assets/127911304/ce81667e-972f-4bf9-b035-069eeacaf28d)

Conociendo el proceso de escritura, pasamos al de lectura. Aca de forma recursiva se va ir reservando memoria y leyendo el contenido del archivo de persistencia para reconstruir el directorio. Siempre se empieza con la estructura `fisopfs_fs_t` para luego de forma recursiva ir leyendo desde la raíz. En cada nivel lo que se va haciendo es leer primero el directorio para luego sus entradas. Aca es cuando `entrada_t` juega un rol importantísimo, porque permite identificar el tipo de entrada que estamos leyendo. Lo que se hace es leer primero el tamaño de una `entrada_t`, a través del campo de `tipo` se identifica si la entrada es un directorio o un archivo, para a continuación retroceder el tamaño recién leído para ahora si leer el tamaño acorde a la estructura. Si lo que leemos es un archivo, se lee primero los metadatos  y a través del campo de `st.st_size` identificamos el tamaño de contenido que hay que leer. Si es un directorio se vuelve a repetir este proceso. Las entradas vacías que no fueron escritas de los directorios, es en este punto en el que se vuelve a reservar y llenar con `entradas_t`. La organización de la estructura no va a quedar exactamente igual pero no afecta en nada a la implementación.  A continuación se deja un diagrama de cómo se reconstruye el sistema de archivos:    

![image](https://github.com/fiubatps/sisop_2023b_g21/assets/127911304/03d45863-3236-4650-b6e6-a4a342c6f17a)
![image](https://github.com/fiubatps/sisop_2023b_g21/assets/127911304/64b8c735-f3aa-4e89-893f-2a87d7730537)



## Limitaciones para el usuario 
La única limitación que nuestro sistema de archivos le impone al usuario es el espacio total ocupado por el sistema de archivos en disco. Esta limitación se nos hizo la mejor de todas, dado que aunque limitamos al usuario sigue permitiéndole muchísima flexibilidad  en cuanto  a qué se puede hacer. Con espacio total ocupado en disco nos referimos al espacio que ocupan tanto datos como metadatos (salvo las entrada_t, que no se escriben en disco). Con este enfoque, el usuario puede elegir tanto si tener muchos archivos  con poco contenido, o un solo archivo enorme con mucho contenido o un directorio con un montón de entradas o como muchos directorios, realmente es muy flexible y el usuario es libre de gestionar su espacio. 
Para llevar este siguiendo del espacio total ocupado en disco, la estructura `fisopfs_fs_t` lleva a cabo un conteo en todo momento de este. Cada vez que se crea un directorio o archivo o se escribe en un archivo, se actualiza el valor del espacio total ocupado en disco. Si al tratar de ejecutar `mkdir`, `create`, `write` o `truncate` no hay espacio suficiente la operación falla. 
Además el usuario puede elegir cuánto espacio puede albergar como máximo el sistema de archivos. Para ello debe correr el proyecto con el flag `--max_espacio_fs <numero>`.

## Ejecucion de pruebas
Como parte del proyecto se nos pidió realizar una serie de pruebas de caja negra sobre lo implementado.
Para eso, creamos el archivo ¨tests.sh¨ que contiene las mencionadas pruebas.

Antes de comenzar con la ejecución de las pruebas, se recomienda ejecutar el siguiente comando:
`$ chmod u+x tests.sh`
Esto permite ejecutar las pruebas con el comando ./tests.sh

Como primer paso, debemos compilar nuestro código con make 

Luego, debemos montar el filesystem en una carpeta. Para eso, dentro de la carpeta que contiene nuestro proyecto, creamos una carpeta ‘prueba’.

En la carpeta que contiene nuestro filesystem, abrimos una terminal y montamos el filesystem en la carpeta ‘prueba’ con el siguiente comando:  
`$ ./fisopfs -f prueba/`

Por último, en otra terminal abierta en la carpeta que contiene nuestro filesystem, ejecutamos las pruebas con el siguiente comando:  
`$ ./tests.sh`

Para observar el resultado óptimo de las pruebas de persistencia, debemos montar nuevamente el filesystem en la carpeta de prueba con:  
`$ ./fisopfs -f prueba/`

Y ejecutar nuevamente las pruebas con:  
`$ ./tests.sh`

## Salida esperada de pruebas
### Prueba creacion de archivo
```
$ touch archivo
> Se creo el archivo correctamente
$ ls
archivo
> Se deberia ver el archivo
```
### Prueba creacion de directorio
```
$ mkdir directorio
> Se creo el directorio correctamente
$ ls
directorio
> Se deberia ver el directorio.
```
### Prueba borrado de directorio
```
$ mkdir directorio
> Se crea el directorio
$ ls
directorio
> Se deberia ver el directorio
$ rmdir directorio
> Se borra el directorio
$ ls
> No se deberia ver el directorio
```
### Prueba borrado de archivo
```
$ touch archivo
> Se crea el archivo
$ ls
archivo
> Se deberia ver el archivo
$ rm archivo
> Se borra el archivo
$ ls
> No se deberia ver el archivo
```
### Prueba creacion directorio de 2do nivel
```
$ mkdir dir1
> Se creo el directorio de 1er nivel
$ mkdir dir1/dir2
> Se crea un directorio de 2do nivel correctamente
$ ls dir1
dir2
> Se deberia ver el directorio 2.
```
### Prueba borrado directorio de 2do nivel
```
$ mkdir dir1
> Se creo el directorio de 1er nivel
$ mkdir dir1/dir2
> Se crea un directorio de 2do nivel
$ ls dir1
dir2
> Se deberia ver el directorio 2.
$ rmdir dir1/dir2
> Se borra el directorio de 2do nivel
$ ls dir1
> No se deberia ver el directorio de 2do nivel
```
### Prueba borrado directorio con contenido
```
$ mkdir dir1
> Se creo el directorio de 1er nivel
$ mkdir dir1/dir2
> Se crea un directorio de 2do nivel
> Se intenta borrar el directorio de 1er nivel
$ rmdir dir1
rmdir: failed to remove 'dir1': Directory not empty
> No se deberia poder borrar un directorio con contenido adentro
```
### Prueba stats
```
$ touch archivo
$ seq 25 > archivo
> Le agrego contenido al archivo
$ stat archivo
# se muestran las stats
> Se muestran las estadisticas del archivo
```
### Prueba stats de acceso y modificacion
```
$ sleep 5
$ touch archivo
$ stat archivo
# se muestran las stats
> Se muestran las estadisticas del archivo
> Vemos que las fechas de acceso y modificacion se actualizaron 5s mas tarde
```
### Prueba lectura de archivos
```
$ echo "Hola como estas" > archivo
$ cat archivo
Hola como estas
> Se lee el contenido del archivo
```
### Prueba lectura de directorios
```
$ mkdir directorio
$ touch archivo
> Se crean un archivo y un directorio
$ ls
directorio archivo
> Se deberian leer un archivo y un directorio
```
### Prueba redireccion escritura
```
$ touch archivo1
$ stat archivo1 > archivo2
> Se escribieron las stats del archivo1 en un archivo2
$ cat archivo2
# se muestran las stats
> Se muestran las estadisticas del archivo1 desde el archivo2
```
### Prueba sobrescribir archivo
```
$ echo "Texto inicial en archivo" > a
$ cat a
Texto inicial en archivo
> Se muestra el texto inicial del archivo
$ echo "Texto sobreescrito en archivo" > a
$ cat a
Texto sobrescrito en archivo
> Se muestra el texto sobreescrito en el archivo
```
### Prueba escribir a continuacion en archivo
```
$ echo "Texto inicial en archivo. " > a
$ cat a
Texto inicial en archivo
> Se muestra el texto inicial del archivo
$ echo "Texto escrito a continuacion en archivo" >> a
$ cat a
Texto inicial en archivo
Texto escrito a continuacion en archivo
> Se muestra el texto inicial junto al escrito a continuacion en el archivo
```
### Prueba persistencia
```
$ ls
archivo_persistencia directorio_persistencia
> Se deberia ver guardado un archivo_persistencia y un directorio_persistencia
$ ls directorio_persistencia
archivo2
> Dentro del directorio_persistencia deberia verse un archivo2
> NOTA: Estos archivos solo se genera luego de la primera ejecucion de las pruebas
> Para ver el resultado optimo, montar nuevamente el filesystem y correr por segunda vez las pruebas
```

## Aclaraciones Finales 
Pese a que la consigna solo pide hasta un nivel de directorios, como la mayoría de funciones nos salieron recursivas el trabajo funciona para múltiples niveles de directorios.
