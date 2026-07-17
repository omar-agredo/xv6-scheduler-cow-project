# Guía de demostración del proyecto xv6

## Objetivo

Demostrar las dos modificaciones principales:

1. scheduler por prioridades con aging;
2. fork con Copy-on-Write.

La demostración se realiza con una CPU para que el comportamiento sea más fácil de observar.

---

# 1. Preparación

Desde el repositorio:

```bash
make clean
make
```

Verificar que la compilación termine correctamente:

```bash
echo $?
```

Resultado esperado:

```text
0
```

Iniciar xv6 con una CPU:

```bash
make qemu CPUS=1
```

---

# 2. Demostración del scheduler

## Prueba de prioridades

Ejecutar:

```text
schedtest
```

La prueba crea tres procesos con prioridades:

| Proceso | Prioridad |
|---|---:|
| Hijo 1 | 0 |
| Hijo 2 | 2 |
| Hijo 3 | 4 |

Explicación para la presentación:

- `0` representa la prioridad más alta.
- `4` representa la prioridad más baja.
- Los procesos de mayor prioridad reciben preferencia.
- La prioridad base se configura mediante `setpriority()`.

Resultado esperado:

```text
=== Priority scheduler test ===
Priority 0 = highest, priority 4 = lowest
All children ready. Releasing workload...
...
=== All children finished ===
```

Los mensajes de inicio pueden aparecer en distinto orden debido a interrupciones y operaciones de consola.

Lo importante es observar el orden de finalización y los ticks.

---

## Prueba de aging

Ejecutar:

```text
agingtest
```

Explicación para la presentación:

- se mantiene activo un proceso de prioridad `0`;
- un proceso de prioridad `4` queda esperando;
- cada 20 ticks de espera mejora temporalmente un nivel;
- para subir de `4` a `0` necesita aproximadamente 80 ticks.

Resultado esperado:

```text
Low-priority process resumed: ... waited=81
High-priority process finished: ...
```

La línea del proceso bajo debe aparecer antes de que termine el proceso de prioridad alta.

Esto demuestra que el aging evita starvation.

---

# 3. Demostración de Copy-on-Write

Ejecutar:

```text
cowtest
```

La prueba utiliza:

- un proceso padre;
- un proceso hijo;
- un proceso nieto;
- 16 páginas de memoria.

Antes de `fork()`, el padre escribe `P`.

Después:

- el hijo escribe `C`;
- el nieto escribe `G`;
- el padre debe conservar `P`.

Resultado esperado:

```text
=== Copy-on-Write test ===
Parent initialized 16 pages with P
Grandchild has private copies: OK
Child has private copies: OK
Parent pages remained unchanged: OK
=== Copy-on-Write test passed ===
```

Explicación para la presentación:

- inicialmente los tres procesos comparten páginas físicas;
- las páginas se marcan como COW y sin permiso de escritura;
- al escribir ocurre un store page fault;
- `cowfault()` crea una copia privada;
- los contadores de referencia evitan liberar páginas todavía compartidas.

---

# 4. Pruebas generales

Ejecutar:

```text
usertests -q
```

Resultado final esperado:

```text
ALL TESTS PASSED
```

Esta suite valida:

- procesos;
- memoria;
- lazy allocation;
- archivos;
- pipes;
- llamadas al sistema;
- errores de direcciones;
- creación y terminación de procesos.

Durante algunas pruebas pueden aparecer mensajes:

```text
usertrap(): unexpected scause
```

Estos mensajes son esperados cuando `usertests` intenta accesos inválidos deliberadamente.

Lo importante es que cada bloque termine en `OK` y que al final aparezca:

```text
ALL TESTS PASSED
```

---

# 5. Archivos que conviene mostrar

## Scheduler

```text
kernel/param.h
kernel/proc.h
kernel/proc.c
kernel/sysproc.c
user/schedtest.c
user/agingtest.c
```

Secciones importantes:

- constantes de prioridad;
- campos `priority` y `ready_since`;
- cálculo de prioridad efectiva;
- syscall `setpriority`;
- prueba de aging.

## Copy-on-Write

```text
kernel/riscv.h
kernel/kalloc.c
kernel/vm.c
kernel/trap.c
user/cowtest.c
```

Secciones importantes:

- `PTE_COW`;
- arreglo `refcount`;
- `kaddref()` y `kgetref()`;
- nueva versión de `uvmcopy()`;
- `cowfault()`;
- integración con `usertrap()` y `copyout()`.

---

# 6. Orden recomendado para el video

1. Presentar el objetivo del proyecto.
2. Mostrar la estructura del repositorio.
3. Explicar brevemente el scheduler original.
4. Mostrar las nuevas prioridades.
5. Ejecutar `schedtest`.
6. Explicar starvation y aging.
7. Ejecutar `agingtest`.
8. Explicar la copia original de `fork()`.
9. Mostrar el diseño Copy-on-Write.
10. Ejecutar `cowtest`.
11. Mostrar `ALL TESTS PASSED`.
12. Presentar conclusiones.

---

# 7. Duración recomendada

| Sección | Tiempo |
|---|---:|
| Introducción | 30–45 segundos |
| Scheduler | 2–3 minutos |
| Aging | 1–2 minutos |
| Copy-on-Write | 2–3 minutos |
| Pruebas y conclusión | 1 minuto |

Duración total recomendada:

```text
7–10 minutos
```

---

# 8. Cierre sugerido

El proyecto modificó dos componentes centrales de xv6.

El scheduler ahora permite prioridades configurables y evita starvation mediante aging.

La administración de memoria optimiza `fork()` con Copy-on-Write, compartiendo páginas hasta que un proceso intenta modificarlas.

Las modificaciones funcionan de forma conjunta y superan la suite completa de pruebas de xv6.
