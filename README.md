# xv6: Scheduler por prioridades con Aging y Copy-on-Write

Proyecto de modificación del sistema operativo educativo **xv6-riscv**.

El proyecto estudia el funcionamiento original de xv6 y añade dos mejoras principales:

1. un scheduler por prioridades con mecanismo de aging;
2. una implementación de `fork()` basada en Copy-on-Write.

Las modificaciones conservan compatibilidad con el comportamiento general de xv6 y fueron validadas mediante pruebas específicas y la suite completa `usertests`.

---

## Objetivos

- Analizar el scheduler original de xv6.
- Diseñar e implementar prioridades para procesos.
- Evitar starvation mediante aging.
- Analizar la administración original de memoria.
- Optimizar `fork()` mediante Copy-on-Write.
- Mantener aislamiento entre procesos.
- Validar las modificaciones mediante pruebas reproducibles.
- Documentar decisiones, resultados y errores encontrados.

---

# 1. Scheduler por prioridades con Aging

## Prioridades

Cada proceso tiene una prioridad base:

| Prioridad | Significado |
|---:|---|
| 0 | Más alta |
| 1 | Alta |
| 2 | Predeterminada |
| 3 | Baja |
| 4 | Más baja |

La prioridad predeterminada es:

```c
#define DEFAULT_PRIORITY 2
```

Los procesos hijos heredan la prioridad base de su padre.

## Aging

Un proceso de baja prioridad podría esperar indefinidamente si siempre existen procesos con prioridad superior.

Para evitarlo, el scheduler calcula una prioridad efectiva:

```text
prioridad efectiva =
prioridad base - tiempo de espera / intervalo de aging
```

Configuración utilizada:

```c
#define PRIORITY_MIN 0
#define PRIORITY_MAX 4
#define DEFAULT_PRIORITY 2
#define AGING_INTERVAL 20
```

Cada 20 ticks de espera en estado `RUNNABLE`, el proceso asciende temporalmente un nivel.

La prioridad base almacenada no se modifica.

## System call

Se añadió:

```c
int setpriority(int priority);
```

Esta llamada permite que un proceso configure su prioridad base entre `0` y `4`.

Ejemplo:

```c
if (setpriority(0) < 0) {
  printf("invalid priority\n");
  exit(1);
}
```

## Resultado del aging

En la prueba realizada:

```text
High-priority process started: tick=9387
Low-priority process entered: tick=9388
Low-priority process resumed: tick=9469 waited=81
High-priority process finished: tick=9507
```

El proceso con prioridad `4` necesitaba ascender cuatro niveles:

```text
4 niveles × 20 ticks = 80 ticks
```

El proceso reanudó después de 81 ticks y antes de que finalizara el proceso de prioridad alta.

Esto demuestra que el aging evita starvation.

---

# 2. Copy-on-Write Fork

## Problema original

La implementación original de `fork()` usa `uvmcopy()` para:

1. reservar nuevas páginas físicas;
2. copiar todas las páginas del padre;
3. mapearlas en el hijo.

Esto puede desperdiciar tiempo y memoria, especialmente cuando el hijo ejecuta `exec()` inmediatamente.

## Solución implementada

Con Copy-on-Write:

```text
fork()
├── padre e hijo comparten las páginas físicas
├── se elimina temporalmente el permiso de escritura
├── las páginas se marcan como COW
└── la copia ocurre únicamente cuando alguien escribe
```

Se utilizó un bit reservado del PTE:

```c
#define PTE_COW (1L << 8)
```

## Contadores de referencia

Cada página física mantiene un contador:

```text
1 referencia  → una estructura usa la página
2 referencias → padre e hijo comparten la página
3 referencias → padre, hijo y nieto comparten la página
0 referencias → la página vuelve a la lista libre
```

Funciones añadidas:

```c
void kaddref(void *);
int kgetref(void *);
```

`kfree()` libera una página físicamente solo cuando el contador llega a cero.

## Resolución del page fault

Cuando un proceso intenta escribir en una página COW:

```text
store page fault
→ usertrap()
→ cowfault()
```

Si la página tiene varias referencias:

```text
reservar una página nueva
→ copiar el contenido
→ mapear la copia privada
→ reducir el contador anterior
→ restaurar permiso de escritura
```

Si solo queda una referencia:

```text
eliminar PTE_COW
→ restaurar PTE_W
→ continuar sin realizar una copia
```

`copyout()` también fue adaptado para resolver páginas COW cuando el kernel escribe en memoria de usuario.

---

# Archivos principales modificados

| Archivo | Cambio |
|---|---|
| `kernel/param.h` | Constantes de prioridad y aging |
| `kernel/proc.h` | Campos `priority` y `ready_since` |
| `kernel/proc.c` | Scheduler, herencia y transiciones de estado |
| `kernel/syscall.h` | Número de `setpriority` |
| `kernel/syscall.c` | Registro de la syscall |
| `kernel/sysproc.c` | Implementación de `sys_setpriority` |
| `kernel/riscv.h` | Bit `PTE_COW` |
| `kernel/kalloc.c` | Contadores de referencia |
| `kernel/vm.c` | `uvmcopy()`, `cowfault()` y `copyout()` |
| `kernel/trap.c` | Resolución de store page faults COW |
| `user/schedtest.c` | Prueba de prioridades |
| `user/agingtest.c` | Prueba de aging |
| `user/cowtest.c` | Prueba de Copy-on-Write |
| `user/usys.pl` | Stub de `setpriority` |
| `user/user.h` | Declaración de `setpriority` |
| `Makefile` | Programas de prueba y compatibilidad |

---

# Compilación

## Requisitos

- QEMU con soporte RISC-V
- Toolchain `riscv64-linux-gnu`
- GNU Make
- Git

El proyecto fue desarrollado y validado en Fedora 44.

## Compilar

```bash
make clean
make
```

## Ejecutar con una CPU

```bash
make qemu CPUS=1
```

## Ejecutar con varias CPUs

```bash
make qemu
```

Para salir de QEMU:

```text
Ctrl+A
X
```

---

# Pruebas

## Scheduler

Dentro de xv6:

```text
schedtest
```

Prueba procesos con prioridades:

```text
0, 2 y 4
```

## Aging

```text
agingtest
```

El proceso de prioridad baja debe reanudar cerca de los 80 ticks y antes de que termine el proceso de prioridad alta.

## Copy-on-Write

```text# xv6: Scheduler por prioridades con Aging y Copy-on-Write

Proyecto de modificación del sistema operativo educativo **xv6-riscv**.

El proyecto estudia el funcionamiento original de xv6 y añade dos mejoras principales:

1. un scheduler por prioridades con mecanismo de aging;
2. una implementación de `fork()` basada en Copy-on-Write.

Las modificaciones conservan compatibilidad con el comportamiento general de xv6 y fueron validadas mediante pruebas específicas y la suite completa `usertests`.

---

## Objetivos

- Analizar el scheduler original de xv6.
- Diseñar e implementar prioridades para procesos.
- Evitar starvation mediante aging.
- Analizar la administración original de memoria.
- Optimizar `fork()` mediante Copy-on-Write.
- Mantener aislamiento entre procesos.
- Validar las modificaciones mediante pruebas reproducibles.
- Documentar decisiones, resultados y errores encontrados.

---

# 1. Scheduler por prioridades con Aging

## Prioridades

Cada proceso tiene una prioridad base:

| Prioridad | Significado |
|---:|---|
| 0 | Más alta |
| 1 | Alta |
| 2 | Predeterminada |
| 3 | Baja |
| 4 | Más baja |

La prioridad predeterminada es:

```c
#define DEFAULT_PRIORITY 2
```

Los procesos hijos heredan la prioridad base de su padre.

## Aging

Un proceso de baja prioridad podría esperar indefinidamente si siempre existen procesos con prioridad superior.

Para evitarlo, el scheduler calcula una prioridad efectiva:

```text
prioridad efectiva =
prioridad base - tiempo de espera / intervalo de aging
```

Configuración utilizada:

```c
#define PRIORITY_MIN 0
#define PRIORITY_MAX 4
#define DEFAULT_PRIORITY 2
#define AGING_INTERVAL 20
```

Cada 20 ticks de espera en estado `RUNNABLE`, el proceso asciende temporalmente un nivel.

La prioridad base almacenada no se modifica.

## System call

Se añadió:

```c
int setpriority(int priority);
```

Esta llamada permite que un proceso configure su prioridad base entre `0` y `4`.

Ejemplo:

```c
if (setpriority(0) < 0) {
  printf("invalid priority\n");
  exit(1);
}
```

## Resultado del aging

En la prueba realizada:

```text
High-priority process started: tick=9387
Low-priority process entered: tick=9388
Low-priority process resumed: tick=9469 waited=81
High-priority process finished: tick=9507
```

El proceso con prioridad `4` necesitaba ascender cuatro niveles:

```text
4 niveles × 20 ticks = 80 ticks
```

El proceso reanudó después de 81 ticks y antes de que finalizara el proceso de prioridad alta.

Esto demuestra que el aging evita starvation.

---

# 2. Copy-on-Write Fork

## Problema original

La implementación original de `fork()` usa `uvmcopy()` para:

1. reservar nuevas páginas físicas;
2. copiar todas las páginas del padre;
3. mapearlas en el hijo.

Esto puede desperdiciar tiempo y memoria, especialmente cuando el hijo ejecuta `exec()` inmediatamente.

## Solución implementada

Con Copy-on-Write:

```text
fork()
├── padre e hijo comparten las páginas físicas
├── se elimina temporalmente el permiso de escritura
├── las páginas se marcan como COW
└── la copia ocurre únicamente cuando alguien escribe
```

Se utilizó un bit reservado del PTE:

```c
#define PTE_COW (1L << 8)
```

## Contadores de referencia

Cada página física mantiene un contador:

```text
1 referencia  → una estructura usa la página
2 referencias → padre e hijo comparten la página
3 referencias → padre, hijo y nieto comparten la página
0 referencias → la página vuelve a la lista libre
```

Funciones añadidas:

```c
void kaddref(void *);
int kgetref(void *);
```

`kfree()` libera una página físicamente solo cuando el contador llega a cero.

## Resolución del page fault

Cuando un proceso intenta escribir en una página COW:

```text
store page fault
→ usertrap()
→ cowfault()
```

Si la página tiene varias referencias:

```text
reservar una página nueva
→ copiar el contenido
→ mapear la copia privada
→ reducir el contador anterior
→ restaurar permiso de escritura
```

Si solo queda una referencia:

```text
eliminar PTE_COW
→ restaurar PTE_W
→ continuar sin realizar una copia
```

`copyout()` también fue adaptado para resolver páginas COW cuando el kernel escribe en memoria de usuario.

---

# Archivos principales modificados

| Archivo | Cambio |
|---|---|
| `kernel/param.h` | Constantes de prioridad y aging |
| `kernel/proc.h` | Campos `priority` y `ready_since` |
| `kernel/proc.c` | Scheduler, herencia y transiciones de estado |
| `kernel/syscall.h` | Número de `setpriority` |
| `kernel/syscall.c` | Registro de la syscall |
| `kernel/sysproc.c` | Implementación de `sys_setpriority` |
| `kernel/riscv.h` | Bit `PTE_COW` |
| `kernel/kalloc.c` | Contadores de referencia |
| `kernel/vm.c` | `uvmcopy()`, `cowfault()` y `copyout()` |
| `kernel/trap.c` | Resolución de store page faults COW |
| `user/schedtest.c` | Prueba de prioridades |
| `user/agingtest.c` | Prueba de aging |
| `user/cowtest.c` | Prueba de Copy-on-Write |
| `user/usys.pl` | Stub de `setpriority` |
| `user/user.h` | Declaración de `setpriority` |
| `Makefile` | Programas de prueba y compatibilidad |

---

# Compilación

## Requisitos

- QEMU con soporte RISC-V
- Toolchain `riscv64-linux-gnu`
- GNU Make
- Git

El proyecto fue desarrollado y validado en Fedora 44.

## Compilar

```bash
make clean
make
```

## Ejecutar con una CPU

```bash
make qemu CPUS=1
```

## Ejecutar con varias CPUs

```bash
make qemu
```

Para salir de QEMU:

```text
Ctrl+A
X
```

---

# Pruebas

## Scheduler

Dentro de xv6:

```text
schedtest
```

Prueba procesos con prioridades:

```text
0, 2 y 4
```

## Aging

```text
agingtest
```

El proceso de prioridad baja debe reanudar cerca de los 80 ticks y antes de que termine el proceso de prioridad alta.

## Copy-on-Write

```text
cowtest
```

La prueba utiliza padre, hijo y nieto compartiendo inicialmente 16 páginas.

Resultado esperado:

```text
=== Copy-on-Write test ===
Parent initialized 16 pages with P
Grandchild has private copies: OK
Child has private copies: OK
Parent pages remained unchanged: OK
=== Copy-on-Write test passed ===
```

## Suite completa

```text
usertests -q
```

Resultado obtenido:

```text
ALL TESTS PASSED
```

También se validó:

```text
forktest
schedtest
agingtest
cowtest
```

---

# Resultados principales

## Scheduler

- La prioridad `0` recibe preferencia frente a `2` y `4`.
- Los procesos de una misma prioridad conservan turnos de ejecución.
- El aging permite que los procesos de baja prioridad progresen.
- Funciona con una y varias CPUs.

## Copy-on-Write

- Padre e hijo comparten inicialmente páginas físicas.
- Las escrituras crean copias privadas.
- Padre, hijo y nieto mantienen contenidos independientes.
- Los contadores de referencia evitan liberaciones prematuras.
- Lazy allocation continúa funcionando.
- La suite general de xv6 termina correctamente.

---

# Tags importantes

```text
baseline-original
baseline-fedora44
scheduler-priority-aging-v1
cow-fork-v1
```

Estos tags permiten comparar el xv6 original con las versiones estables de cada modificación.

Ejemplo:

```bash
git checkout baseline-original
```

Para regresar a la versión final:

```bash
git checkout feature/scheduler-priority-aging
```

---

# Errores encontrados y correcciones

## Compatibilidad con GCC 16

La compilación original trataba una advertencia de `usertests.c` como error.

Se añadió:

```text
-Wno-error=unused-but-set-variable
```

## Dirección superior a MAXVA

Durante `MAXVAplus` apareció:

```text
panic: walk
```

La causa era que `cowfault()` llamaba a `walk()` con una dirección virtual inválida.

Se corrigió con:

```c
if (va >= MAXVA)
  return 0;
```

Después de la corrección:

```text
ALL TESTS PASSED
```

---

# Estructura de documentación

La documentación complementaria se mantiene en el directorio hermano:

```text
docs/
├── evidence/
├── notes/
└── results/
```

Incluye:

- análisis del scheduler original;
- diseño del scheduler;
- resultados de prioridades y aging;
- análisis de memoria;
- resultados de Copy-on-Write;
- capturas de compilación y pruebas.

---

# Autor

Proyecto académico de Sistemas Operativos.

Desarrollado sobre xv6-riscv con fines educativos.

---

# Créditos de xv6

xv6 es una reimplementación educativa de Unix Version 6 desarrollada por MIT para la enseñanza de sistemas operativos.

Sitio oficial del curso:

```text
https://pdos.csail.mit.edu/6.1810/
```

El código base y sus autores originales conservan sus respectivos créditos.
cowtest
```

La prueba utiliza padre, hijo y nieto compartiendo inicialmente 16 páginas.

Resultado esperado:

```text
=== Copy-on-Write test ===
Parent initialized 16 pages with P
Grandchild has private copies: OK
Child has private copies: OK
Parent pages remained unchanged: OK
=== Copy-on-Write test passed ===
```

## Suite completa

```text
usertests -q
```

Resultado obtenido:

```text
ALL TESTS PASSED
```

También se validó:

```text
forktest
schedtest
agingtest
cowtest
```

---

# Resultados principales

## Scheduler

- La prioridad `0` recibe preferencia frente a `2` y `4`.
- Los procesos de una misma prioridad conservan turnos de ejecución.
- El aging permite que los procesos de baja prioridad progresen.
- Funciona con una y varias CPUs.

## Copy-on-Write

- Padre e hijo comparten inicialmente páginas físicas.
- Las escrituras crean copias privadas.
- Padre, hijo y nieto mantienen contenidos independientes.
- Los contadores de referencia evitan liberaciones prematuras.
- Lazy allocation continúa funcionando.
- La suite general de xv6 termina correctamente.

---

# Tags importantes

```text
baseline-original
baseline-fedora44
scheduler-priority-aging-v1
cow-fork-v1
```

Estos tags permiten comparar el xv6 original con las versiones estables de cada modificación.

Ejemplo:

```bash
git checkout baseline-original
```

Para regresar a la versión final:

```bash
git checkout feature/scheduler-priority-aging
```

---

# Errores encontrados y correcciones

## Compatibilidad con GCC 16

La compilación original trataba una advertencia de `usertests.c` como error.

Se añadió:

```text
-Wno-error=unused-but-set-variable
```

## Dirección superior a MAXVA

Durante `MAXVAplus` apareció:

```text
panic: walk
```

La causa era que `cowfault()` llamaba a `walk()` con una dirección virtual inválida.

Se corrigió con:

```c
if (va >= MAXVA)
  return 0;
```

Después de la corrección:

```text
ALL TESTS PASSED
```

---

# Estructura de documentación

La documentación complementaria se mantiene en el directorio hermano:

```text
docs/
├── evidence/
├── notes/
└── results/
```

Incluye:

- análisis del scheduler original;
- diseño del scheduler;
- resultados de prioridades y aging;
- análisis de memoria;
- resultados de Copy-on-Write;
- capturas de compilación y pruebas.

---

# Autor

OMAR ESTEBAN AGREDO

Proyecto académico de Sistemas Operativos.

Desarrollado sobre xv6-riscv con fines educativos.

---

# Créditos de xv6

xv6 es una reimplementación educativa de Unix Version 6 desarrollada por MIT para la enseñanza de sistemas operativos.

Sitio oficial del curso:

```text
https://pdos.csail.mit.edu/6.1810/
```

El código base y sus autores originales conservan sus respectivos créditos.
