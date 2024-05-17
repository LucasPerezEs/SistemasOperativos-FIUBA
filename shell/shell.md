# shell

### Búsqueda en _$PATH_
**¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?**

La syscall execve(2) reemplaza en memoria el programa que está siendo ejecutado por un nuevo programa que recibe como parámetro (pathname). Este parámetro debe ser un path a un binario ejecutable o un script. El nuevo programa en ejecución tendrá nuevos heap y stack inicializados. Cabe destacar que el proceso “llamador” sigue siendo el mismo por lo cual el PID no cambia.
Además execve(2) recibe dos arreglos de cadenas (argv y envp) que se le pasarán al nuevo programa como parámetros y environment respectivamente.

La familia de wrappers proporcionados por la librería estándar de C exec(3) funcionan como front-end para la syscall execve(2). Es decir, por dentro llaman a la syscall pero agregando por fuera cierta personalización y funcionalidades para ajustarse mejor a las necesidades del programador al momento de querer llamarla. Estas están agrupadas según las letras que preceden al prefijo “exec”:
- La letra “l” indica que los argumentos son pasados en forma de lista de punteros a string. 
- La letra “v” indica que los argumentos son pasados en forma de vector de strings, al igual que el que recibe execve(2). 
- La letra “e” indica que el environment del proceso llamador se especifica mediante el argumento envp. Las otras funciones de exec(3) que no contienen la “e” toman el environment para el nuevo programa de la variable externa environ en el proceso “llamador”.
- La letra “p” indica la posibilidad de pasar como parámetro directamente el nombre del binario ejecutable deseado sin la necesidad de indicar el path completo. En este caso se busca el ejecutable dentro de la lista de directorios especificada por la variable de entorno PATH, la cual por defecto suele contener “/bin:/usr/bin:/usr/local/bin” entre otros.
  En caso de pasarse una string que contenga el carácter “/” se ignora el PATH y se utiliza el ejecutable dentro del path recibido.


En particular nosotros utilizamos la función execvp(3) que al tener las letras “v” y “p” implementa el comportamiento descrito previamente.

---
**¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?** 

La llamada a exec(3) puede fallar por los mismos motivos que la llamada a la syscall subyacente puede hacerlo. Estos son bastantes de modo que nos limitaremos a mencionar algunos:

- El path proporcionado es inválido, por ejemplo, porque alguno de los directorios no es un directorio 
- El ejecutable no es reconocido como ejecutable por tener formato o arquitectura incorrecta o directamente no ser un archivo regular
- Error de I/O
- No se tiene acceso al ejecutable pasado
- El número total de bits de envp y argv es demasiado grande

En caso de error la función devuelve -1 en cuyo caso se imprime el error por consola con el texto “Fallo en el exec” y se corta la ejecución de la shell mediante “_exit(ERROR)”.

### Procesos en segundo plano
**Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.**

El mecanismo utilizado para implementar procesos en segundo plano en esta primera parte fue el siguiente: se distinguen los procesos en segundo plano del resto. A los procesos "normales" la shell les hace un wait bloqueante para esperar su finalización. A los procesos en segundo plano no se les hace esperar, devolviendo el command prompt inmediatamente al usuario una vez hecho el exec. Ahora bien, para esperar de forma oportuna a la finalización de estos procesos y que no queden zombies, en cada iteración la shell realiza un waitpid no bloqueante (con el flag WNOHANG) para ver si alguno de los procesos hijos en segundo plano ya finalizó. De esta manera, si algún proceso en segundo plano ya finalizó su ejecución, con este waitpid logramos liberar sus recursos y si por el contrario, ninguno finalizó, la shell puede seguir con su ejecución dado que el waitpid retorna inmediatamente.

La problemática con esta implementación es que no se puede liberar de forma inmediata los procesos en segundo plano ya finalizados. 

## Parte 2: Redirecciones 
### Flujo estándar
**Investigar el significado de 2>&1, explicar cómo funciona su forma general**

- **Mostrar qué sucede con la salida de cat out.txt en el ejemplo.** 
- **Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).**

En la terminal, un comando que incluya  la serie  2>& 1 vendría a expresar: que el file descriptor número 2 del programa tenga como salida la misma que tiene asociado el file descriptor 1. Claro está que los file descriptor 1 y 2 de un proceso, son sdtout y stderr, respectivamente, por lo tanto lo que se está haciendo, es modificar la salida de error del programa para que sea la misma que la de salida estándar. Llevada a su forma general, la expresión seria ‘n>&m’, indicando que la salida del file descriptor n sea la misma que la que tiene asociado el file descriptor m.


Al ejecutar en nuestra shell implementada el comando de ejemplo obtenemos: 

```sh
$ ls -C /home /noexiste >out.txt 2>&1

$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
siro
```
El resultado obtenido es el mismo que el de bash. 
Ahora si ejecutamos con el orden de las redirecciones invertidas el resultado no cambia. Sin embargo, el resultado es diferente del de bash, que devuelve:   
```sh
$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
$ cat out.txt
/home:
siro
```
Esta diferencia no hace más que revelar la divergencia de implementación entre nuestra shell con respecto a la de bash. En nuestra implementación, sin importar el orden en que aparecen las redirecciones en el comando, redirigimos antes la salida estándar que la salida de error estándar, por lo tanto, el orden de los factores no altera el producto. Es indiferente para nuestra shell `2>&1 >out.txt` que `>out.txt 2>&1 `, porque internamente redirigimos antes la salida estándar a out.txt y después,  modificamos la salida de error de estándar, quedando tanto el error como la salida estándar redirigidos hacia out.txt. Por su parte, en bash parece que el orden de las redirecciones si afecta al producto, generando disparidad en el resultado. Entonces, como si importar el orden, cuando bash se topa con `2>&1 >out.txt`, primero cambia el flujo de la salida de error estándar a la misma que la salida estándar, que como todavía no se modificó sigue siendo la pantalla y después cambia la salida estándar al archivo out.txt.

### Tuberias
**Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe**
- **¿Cambia en algo?** 
- **¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.**

Cuando se ejecuta un pipe, el exit code reportado por bash va a ser el del último proceso ejecutado (el que esté más a la derecha en la cadena de pipes). Esto lo podemos ver con un breve ejemplo y usando la pseudo-variable implementada `?`.
```sh
$ ls | sleep 1 | algoquenoexiste
algoquenoexiste: command not found
$ echo $?
127
```
Como vemos, el exit code del comando ejecutado es el código de error del último proceso ejecutado. 
Ahora, si invertimos el orden de ejecución de comandos, el exit code es el siguiente:
```sh
$ ls | algoquenoexiste | sleep 1
algoquenoexiste: command not found
$ echo $?
0
```
En este caso, aunque haya un error en el proceso del medio, el exit code sigue siendo el del último proceso ejecutado.

En contraste, en nuestra implementación de shell, ejecutar el comando `$ echo $?` obtendremos siempre `0` como respuesta, sin importar si todo funcionó correctamente o si alguno de los comandos del pipe falló. Esto sucede porque al ejecutar un comando con pipes internamente se hacen múltiples forks para ejecutar los distintos comandos individuales. Al hacerlo no estamos propagando el exit code del último proceso en ejecutarse. 

## Parte 3: Variables de entorno
### Variables de entorno temporarias
**¿Por qué es necesario hacerlo luego de la llamada a fork(2)?**

Las variables de entorno temporarias nos permiten cambiar de manera dinámica el valor de las variables de entorno para la ejecución de un programa. Para lograr esta funcionalidad es necesario modificar las variables de entorno del programa que se va a invocar con exec, sin modificar las de shell, teniendo dos posibles caminos: modificar las variables antes de la llamada a fork o después. Si comparamos, el mejor camino es sin duda la segunda opción. El primer camino es más complejo y requiere más pasos:  previo a modificar las variables de entorno, habría que guardar sus valores previos, dado que como todavía no hicimos fork y, por lo tanto, no creamos un nuevo proceso, al modificar los valores estaríamos cambiando los de la shell en general y perdiéndose de no guardarlos. Además, una vez hecho el fork, desde el lado de la shell habría que volver a setear los valores previos al cambio. Por su parte, el segundo camino es mucho más sencillo: como ya se hizo el fork y se creó un nuevo proceso, se puede cambiar los valores sin necesidad de guardar el valor previo, dado que, como nos encontramos en un nuevo proceso, la modificación de estos no afecta a los de la shell y solo existe en el propio proceso. Por esto, resulta más conveniente cambiar los valores luego a la llamada a fork.  

---
**En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).**
- **¿El comportamiento resultante es el mismo que en el primer caso?         Explicar qué sucede y por qué.** 
-  **Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**
-  
Si en vez de usar setenv, usaramos las funciones execve, execvpe o execle, el comportamiento no seria estrictamente el mismo.  Al usar setenv, lo que buscamos es setear las variables de entorno temporarias en el proceso hijo previo a la llamada de exec, modificando o agregando estas variables al programa a ser invocado y a su vez, que mantenga todas las previas del proceso padre, la shell. Es decir, con setenv logramos modificar e insertar de forma efímera en el programa solo las variables de entorno señaladas y que se hereden  todas las demás variables. Cosa distinta sucedería si usaramos execve o execle y les pasaremos como tercer argumento las variables de entorno recibidas en el comando. Con esto, el programa invocado tendría configurado solo estas variables de entorno  pero le faltarían todas las demás y no heredaría las de la shell.

Si quisiéramos que el comportamiento sea el mismo en ambos casos, al momento de usar execve o  execle, además de mandar las variables recibidas en el comando, habría que mandar todo el resto que ya tiene configurada la shell. Si este fuera el caso, sería importante primero enviar todas las de shell y después enviar las propias del comando, dado que de esta manera, si se buscara modificar alguna de las variables, prevalecia la modificación, quedando como valor el último asociado a determinada clave.          
### Pseudo-variables
**Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).**

- `$$` muestra el PID de la shell. Ej: `ps -p $$`
- `$#` Devuelve el número de argumentos que fueron pasados a un script o     función. Ej: 
```sh
if [ $# == 0 ]; then
    echo "$0 filename"
    exit 1
```
- `$@` devuelve todos los argumentos pasados a un script. Ej:
```sh
echo “Los argumentos pasados al script son: “
echo “$@”
```
## Parte 4: comandos _built_in_
**¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)** 

Si, pwd se podría implementar sin la necesidad de built-in. Por un lado, implementar cd sería imposible sin el built-in, dado que no hay manera que un programa le cambie el directorio actual a la shell. Por otro lado, pwd si se podría llegar a implementar mediante un binario. La razón por la que se hace built-in es por un tema de performance: porque dejar que lo haga otro cuando la shell puede hacerlo sencillamente. Llamar un programa requiere usar fork y exec, operaciones nada baratas desde la perspectiva del rendimiento y, por ende, es mejor optar por que la shell misma se encargue, rol que puede cumplir de manera suficiente.

Cosa similar pasa con true y false, sería ridículamente fácil hacer un par de programas que lo único que hagan sea terminar con éxito y error, respectivamente. Sin embargo, se opta por hacerlos un built-in por un tema de performance, que como ya se mencionó, fork y exec no son nada baratos y es mejor optar por que la shell se encargue de la labor.           

## Parte 5: Segundo plano avanzado
**¿Por qué es necesario el uso de señales?**

Para mejorar el seguimiento y manejo de los procesos en segundo plano se vuelve necesario el uso de señales. Con esta implementación mejorada buscamos que los procesos se liberen ni bien termine de ejecutarse el proceso hijo y no estar constantemente preguntando con waitpid si ya terminó. Para lograr esto el uso de señales es necesario, dado que es por este medio que el S.O. se comunica con los diferentes procesos y les avisa de la finalización de los procesos hijos con la señal SIGCHLD. De esta forma, cada vez que la shell recibe la señal SIGCHLD, sabe que finalizó un proceso en segundo plano y le puede hacer un wait no bloqueante para liberar sus recursos.  Sin el uso de señales, no habría otra forma de poder avisarle de forma asincrónica a los procesos cuando finalizan sus procesos hijos.
 

