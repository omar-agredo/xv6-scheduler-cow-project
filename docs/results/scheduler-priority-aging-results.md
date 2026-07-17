# Resultados del scheduler por prioridades con aging

## Configuración

- Arquitectura: RISC-V
- Sistema: xv6-riscv
- CPUs principales de prueba: 1
- Prioridad mínima: 0
- Prioridad máxima: 4
- Prioridad predeterminada: 2
- Intervalo de aging: 20 ticks

## Prueba de prioridades

Se ejecutaron tres procesos intensivos en CPU con prioridades:

| Proceso | Prioridad base | Resultado |
|---|---:|---|
| Hijo 1 | 0 | Finalizó primero |
| Hijo 2 | 2 | Finalizó segundo |
| Hijo 3 | 4 | Finalizó después de los procesos superiores |

Resultado observado:

```text
Child 1 priority=0 finished at tick 9321
Child 2 priority=2 finished at tick 9326
Child 3 priority=4 finished at tick 9331
