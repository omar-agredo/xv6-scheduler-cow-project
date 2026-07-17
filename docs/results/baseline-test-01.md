# Prueba BASE-01 — Arranque del xv6 original

## Objetivo

Comprobar que la versión original de xv6-riscv puede compilarse y ejecutarse
correctamente en el entorno preparado.

## Configuración

- Sistema anfitrión: Fedora 44 x86_64
- Emulador: QEMU RISC-V
- Versión de xv6: consultar `xv6-source-version.txt`
- Línea base: `baseline-fedora44`
- Comando de ejecución: `make qemu`

## Procedimiento

1. Compilar el proyecto.
2. Verificar la generación de `kernel/kernel`.
3. Verificar la generación de `fs.img`.
4. Ejecutar `make qemu`.
5. Esperar el inicio del shell de xv6.
6. Ejecutar `ls`.
7. Ejecutar `echo hola xv6`.
8. Salir de QEMU con `Ctrl+A`, seguido de `X`.

## Resultado esperado

- La compilación termina sin errores.
- xv6 muestra el mensaje de arranque.
- Se inicia el shell.
- `ls` muestra los archivos y programas disponibles.
- `echo hola xv6` imprime el texto esperado.

## Resultado obtenido

La compilación y el arranque fueron exitosos. El shell respondió correctamente
a los comandos de prueba.

## Estado

APROBADA.

## Evidencias

- `02-xv6-compilado.png`
- `03-xv6-original-ejecutandose.png`
- `final-diagnostic.txt`
- `xv6-source-version.txt`
