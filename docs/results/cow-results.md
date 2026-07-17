# Resultados de Copy-on-Write Fork

## Objetivo

Modificar `fork()` para que padre e hijo compartan inicialmente las páginas físicas, evitando copiar toda la memoria de manera inmediata.

Las páginas modificables se marcan como solo lectura y Copy-on-Write. Cuando un proceso intenta escribir, el kernel crea una copia privada.

## Diseño implementado

### Bit COW

Se añadió un bit reservado del PTE:

```c
#define PTE_COW (1L << 8)
