# Diseño del scheduler por prioridades con aging

## Objetivo

Modificar el scheduler original de xv6 para seleccionar primero los procesos
con mayor prioridad, evitando starvation mediante un mecanismo de aging.

## Rango de prioridades

- `0`: prioridad más alta.
- `4`: prioridad más baja.
- Prioridad predeterminada: `2`.

## Prioridad base y efectiva

Cada proceso tendrá:

- `base_priority`: prioridad normal.
- `effective_priority`: prioridad después de aplicar aging.
- `ready_since`: tick desde el cual espera en estado `RUNNABLE`.

El aging modifica únicamente la prioridad efectiva. Cuando el proceso obtiene
CPU, la prioridad efectiva vuelve a la prioridad base.

## Aging

Cuando un proceso permanece en estado `RUNNABLE`, se calcula:

`waiting_time = ticks - ready_since`

Por cada intervalo de aging transcurrido, su prioridad efectiva mejora, sin
superar la prioridad máxima `0`.

Valor inicial propuesto:

`AGING_INTERVAL = 20 ticks`

Este valor deberá validarse experimentalmente.

## Selección

El scheduler seleccionará:

1. el proceso `RUNNABLE` con menor `effective_priority`;
2. en caso de empate, el proceso con menor `ready_since`.

## Herencia

Los procesos hijos heredarán la prioridad base del padre.

## Archivos previstos

- `kernel/proc.h`
- `kernel/proc.c`

Podrían añadirse archivos relacionados con llamadas al sistema si se implementa
una interfaz para consultar o modificar prioridades.

## Riesgos

- starvation si el aging es incorrecto;
- carreras al consultar `ticks`;
- errores de locks;
- actualización repetida del aging;
- pérdida de equidad entre procesos de igual prioridad.

## Ciclo de creación de procesos

`allocproc()` busca una entrada `UNUSED`, le asigna un PID y cambia su estado
a `USED`. También crea su trapframe, tabla de páginas y contexto inicial.

`userinit()` utiliza `allocproc()` para crear el primer proceso y lo cambia a
`RUNNABLE`.

`kfork()` crea un hijo, copia los recursos del padre y finalmente cambia el
estado del hijo a `RUNNABLE`.

Para el scheduler propuesto:

- la prioridad predeterminada se inicializará durante la creación;
- el hijo heredará la prioridad base del padre;
- la prioridad efectiva comenzará igual a la prioridad base;
- el tiempo de espera deberá registrarse cuando el proceso pase realmente a
  `RUNNABLE`;
- los campos añadidos deberán reiniciarse en `freeproc()`.

## Transiciones hacia `RUNNABLE`

En la versión analizada existen cinco puntos donde un proceso entra o vuelve al
estado `RUNNABLE`:

- `userinit()`: crea el primer proceso;
- `kfork()`: activa un proceso hijo;
- `yield()`: el proceso actual entrega la CPU;
- `wakeup()`: un proceso termina su espera por un evento;
- `kkill()`: un proceso dormido es despertado para poder terminar.

En cada transición deberá registrarse el tick desde el cual el proceso vuelve a
esperar CPU.

El tiempo en estado `SLEEPING` no debe contarse como tiempo de espera del
scheduler, ya que el proceso no estaba listo para ejecutar.

Cuando un proceso vuelva a `RUNNABLE`, su prioridad efectiva deberá comenzar
nuevamente desde su prioridad base. El aging se aplicará solo durante la espera
continua en `RUNNABLE`.

## Tabla de procesos y complejidad

La tabla global de procesos está declarada como:

`struct proc proc[NPROC]`

En la versión analizada, `NPROC` tiene el valor `64`.

`procinit()` inicializa el lock de cada entrada, establece su estado como
`UNUSED` y asigna su pila de kernel.

El scheduler original ya realiza un recorrido lineal de esta tabla. Por tanto,
seleccionar el proceso con mejor prioridad mediante otro recorrido lineal
mantiene una complejidad de `O(NPROC)`.

Dado que el máximo es de 64 procesos, esta estrategia es suficiente para el
alcance académico del proyecto y evita estructuras más complejas.

## Configuración multiprocesador

La constante `NCPU` permite hasta 8 CPUs, mientras que el Makefile inicia QEMU
con 3 CPUs virtuales por defecto mediante `CPUS := 3`.

Cada CPU ejecuta su propia instancia de `scheduler()` y comparte la tabla
global `proc[]`. Los locks por proceso evitan que dos CPUs seleccionen o
modifiquen simultáneamente el mismo proceso.

Las pruebas iniciales del scheduler se realizarán con `CPUS=1` para obtener
resultados deterministas. Después se validará el funcionamiento con las 3 CPUs
predeterminadas.

## Estrategia de selección y locking

Para evitar conservar un candidato después de liberar su lock, el scheduler
recorrerá los niveles de prioridad efectiva desde `0` hasta `4`.

Para cada nivel recorrerá la tabla `proc[]`. Cada proceso será inspeccionado
manteniendo únicamente su propio `p->lock`. Si está en `RUNNABLE` y su prioridad
efectiva coincide con el nivel actual, se cambiará inmediatamente a `RUNNING`
y se realizará el cambio de contexto.

Esta estrategia conserva el patrón de locking del scheduler original y evita
que dos CPUs seleccionen el mismo proceso.

La prioridad efectiva se calculará dinámicamente a partir de:

- prioridad base del proceso;
- tick desde el cual espera en `RUNNABLE`;
- intervalo de aging.

No se almacenará un campo separado de prioridad efectiva.
