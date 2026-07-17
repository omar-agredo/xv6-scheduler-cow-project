# Prueba SCHED-BASE-01 — Scheduler original con una CPU

## Objetivo

Observar el comportamiento del scheduler original de xv6 al ejecutar varios
procesos con la misma carga sobre una sola CPU virtual.

## Configuración

- Línea base: `baseline-fedora44`
- CPUs virtuales: `1`
- Comando de arranque: `make qemu CPUS=1`
- Programa de prueba: `schedtest`
- Número de procesos hijos: `3`
- Tipo de carga: intensiva en CPU

## Procedimiento

1. Compilar xv6 con el programa `schedtest`.
2. Ejecutar xv6 con una sola CPU.
3. Ejecutar `schedtest` desde el shell.
4. Registrar el orden de inicio y finalización de los procesos.

## Resultado obtenido

Orden de inicio:

1. Child 1 — PID 4
2. Child 2 — PID 5
3. Child 3 — PID 6

Orden de finalización:

1. Child 1 — PID 4
2. Child 2 — PID 5
3. Child 3 — PID 6

## Interpretación preliminar

Los tres procesos utilizaron la misma política y no tenían prioridades
diferenciadas. Con una sola CPU y cargas equivalentes, el orden observado fue
consistente con el recorrido secuencial de la tabla de procesos.

Esta prueba no demuestra por sí sola todas las propiedades del scheduler,
pero establece un comportamiento inicial para comparar con la versión
modificada.

## Evidencia

- `04-scheduler-baseline-cpu1.png`

## Estado

APROBADA.

## Repeticiones adicionales

Se ejecutó `schedtest` tres veces consecutivas con una sola CPU virtual.

| Ejecución | Orden de finalización |
|---|---|
| 1 | Child 1 → Child 2 → Child 3 |
| 2 | Child 1 → Child 2 → Child 3 |
| 3 | Child 1 → Child 2 → Child 3 |

Los resultados fueron consistentes en las tres repeticiones.

En la segunda ejecución se observó una superposición de texto en la consola
durante la impresión de dos mensajes. Esto corresponde a salidas concurrentes
y no indica un error del scheduler.
