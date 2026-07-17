# Preparación del entorno de xv6

## Sistema anfitrión

- Distribución: Fedora 44
- Arquitectura: x86_64
- Emulador: QEMU para RISC-V
- Toolchain: GCC y Binutils para riscv64-linux-gnu

## Objetivo

Preparar un entorno capaz de compilar y ejecutar xv6-riscv sin modificar
todavía el funcionamiento del scheduler ni del manejo de memoria.

## Herramientas utilizadas

- Git
- GNU Make
- QEMU RISC-V
- GCC RISC-V
- Binutils RISC-V
- GDB

## Problema encontrado durante la compilación

La primera compilación logró generar `kernel/kernel`, pero se detuvo al
compilar `user/usertests.c`.

El compilador GCC 16 reportó la advertencia:

`variable 'fsblocks' set but not used`

El Makefile de xv6 utiliza la opción `-Werror`, que convierte todas las
advertencias en errores. Por esta razón no se generó inicialmente `fs.img`
y xv6 no pudo completar el arranque.

## Solución aplicada

Se añadió al Makefile la opción:

`-Wno-error=unused-but-set-variable`

La opción conserva la advertencia, pero evita que esta advertencia específica
detenga la compilación.

Este cambio se considera un ajuste de compatibilidad con el entorno y no una
modificación funcional del scheduler ni del manejo de memoria.

## Resultado

Después del ajuste:

- se generó `kernel/kernel`;
- se generó `fs.img`;
- xv6 arrancó correctamente mediante `make qemu`;
- apareció el shell de xv6;
- se ejecutaron correctamente los comandos `ls` y `echo hola xv6`.

## Estado de la línea base

La versión original de xv6 funciona en el entorno preparado. Todavía no se han
realizado modificaciones al planificador de CPU ni al manejo de memoria.
