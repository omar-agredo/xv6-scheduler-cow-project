# Estructuras relacionadas con el scheduler original

## `enum procstate`

Define los estados posibles de un proceso:

- `UNUSED`: entrada libre.
- `USED`: proceso en preparación.
- `SLEEPING`: proceso bloqueado esperando un evento.
- `RUNNABLE`: proceso listo para usar CPU.
- `RUNNING`: proceso actualmente en ejecución.
- `ZOMBIE`: proceso terminado cuyo padre aún no ha recogido el estado.

El scheduler solamente puede seleccionar procesos en estado `RUNNABLE`.

## `struct context`

Guarda los registros necesarios para realizar cambios de contexto dentro del
kernel. Incluye el return address, el stack pointer y los registros preservados
`s0` a `s11`.

## `struct cpu`

Representa el estado de una CPU. Contiene:

- un puntero al proceso que ejecuta actualmente;
- el contexto del scheduler;
- información sobre el estado de las interrupciones.

## `struct proc`

Representa un proceso dentro del kernel. Los campos más relacionados con el
scheduler son:

- `lock`;
- `state`;
- `chan`;
- `killed`;
- `pid`;
- `context`.

Los campos `sz`, `pagetable` y `trapframe` serán relevantes en el análisis del
manejo de memoria.

## Preempción por temporizador

La preempción ocurre en `kernel/trap.c`.

Tanto `usertrap()` como `kerneltrap()` verifican si `devintr()` identificó
una interrupción de temporizador mediante el valor `which_dev == 2`.

Cuando existe un proceso actual, se llama a `yield()`. Esta función cambia el
estado del proceso de `RUNNING` a `RUNNABLE` y llama a `sched()` para devolver
el control al scheduler.

De esta forma, un proceso no puede conservar la CPU indefinidamente, aunque no
la entregue voluntariamente.

## Hallazgo preliminar sobre memoria

La versión oficial utilizada ya contiene una llamada a `vmfault()` desde
`usertrap()` para atender page faults asociados con páginas asignadas de forma
diferida.

Por lo tanto, antes de implementar la modificación de memoria propuesta, será
necesario analizar el lazy allocation existente y ajustar la propuesta para
evitar duplicar funcionalidad ya incluida en la línea base.

## Identificación de la interrupción del temporizador

La función `devintr()`, definida en `kernel/trap.c`, examina el registro
`scause` para determinar el origen de una interrupción.

Sus valores de retorno son:

- `2`: interrupción del temporizador;
- `1`: interrupción de otro dispositivo;
- `0`: interrupción no reconocida.

Cuando `scause` corresponde a una interrupción de temporizador, `devintr()`
llama a `clockintr()` y devuelve `2`.

Posteriormente, `usertrap()` o `kerneltrap()` comprueban `which_dev == 2` y
llaman a `yield()`. De esta manera, el proceso actual cambia de `RUNNING` a
`RUNNABLE` y devuelve el control al scheduler.

## Contador global de tiempo

La función `clockintr()` administra el contador global `ticks`.

Solo la CPU con identificador `0` incrementa este contador. La actualización
se protege mediante `tickslock` y después se llama a `wakeup(&ticks)` para
despertar procesos que esperan el avance del reloj.

La siguiente interrupción se programa mediante `w_stimecmp()`.

El contador `ticks` puede utilizarse como referencia temporal para medir cuánto
tiempo lleva un proceso en estado `RUNNABLE`. Sin embargo, recorrer toda la tabla
de procesos en cada interrupción podría introducir overhead, por lo que conviene
evaluar una estrategia de aging calculada durante la selección del scheduler.

## Uso de `ticks` para medir tiempo

La llamada al sistema `sys_pause()` utiliza el contador global `ticks` para
medir el paso del tiempo.

Primero guarda el valor inicial en `ticks0`. Después, mientras la diferencia
`ticks - ticks0` sea menor que el número solicitado, el proceso se duerme sobre
el canal `&ticks`.

Cada interrupción del temporizador incrementa `ticks` y ejecuta
`wakeup(&ticks)`, lo que permite que los procesos dormidos vuelvan a comprobar
si ya transcurrió el tiempo solicitado.

Este patrón demuestra que `ticks` puede utilizarse como referencia temporal
para calcular cuánto tiempo lleva un proceso esperando en estado `RUNNABLE`.
