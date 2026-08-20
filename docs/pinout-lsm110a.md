# Asignación de pines — LSM110A en 0G LockControl

> **Estado:** tabla parcialmente corregida. La fase F1 detectó 4 pines mal asignados
> (hallazgo H-01) y aquí se aplican esas 4 correcciones. La tabla completa de 34 pines,
> verificada contra la Tabla 5-1-1 del DS R08, está en
> `Hardware/v0-replica-sji/00-fuente-de-verdad/pinout-34-pines.md` §1 y **debe
> reemplazar esta tabla entera** cuando ese archivo entre al repo (decisión D-04).
> Hasta entonces, considerar autoritativa la tabla de F1, no esta.

| Pin LSM110A | GPIO STM32WL | Función | Configuración | Notas |
|-------------|-------------|---------|---------------|-------|
| 11 | VDD | Alimentación | 3.0V directo o 2.5V LDO | 100nF + 10µF desacoplo |
| 1,10,12,20,23,32,34 | GND | Tierra | Plano GND | Todos conectados |
| 7 | PA13 | SWDIO | Debug SWD | Header 4 pines |
| 8 | PA14 | SWCLK | Debug SWD | Header 4 pines |
| 3 | PA9 | I2C1_SCL | Open-drain + pullup 4.7kΩ | Acelerómetro LIS2DW12 — corregido (H-01) |
| 4 | PA10 | I2C1_SDA | Open-drain + pullup 4.7kΩ | Acelerómetro LIS2DW12 — corregido (H-01) |
| 16 | PA0 | EXTI0 wake-up | Input, pull-down | INT1 del LIS2DW12 — corregido (H-01) |
| 15 | PA1 | EXTI1 wake-up | Input | Reed switch o DRV5032 |
| 18 | PB7 | UART1_RX | Input (debug) | No poblar en producción |
| 19 | PB6 | UART1_TX | Output (debug) | No poblar en producción |
| 14 | PA2 | GPIO | Output push-pull | LED debug — corregido (H-01) |
| 30 | NRST | Reset | Cap 100nF a GND | Botón reset opcional |
| 31 | BOOT0 | Boot mode | Flotante (pulldown interno) | No conectar |
| 33 | RF_OUT | Salida RF 50Ω | Traza CPWG 50 Ω (1.0 mm / gap 0.15 mm) | Antena PCB diseño SJI |

## Notas de diseño
- Los pines 26 y 27 quedan sin asignar tras la corrección H-01 (antes se les atribuía
  I2C1_SCL/SDA). Su función real se toma de `pinout-34-pines.md` §1, no de aquí.
- Todos los pines GND deben conectarse al plano de tierra sólido
- Capacitores de desacoplo lo más cerca posible de pin VDD
- Traza RF: CPWG 50 Ω, 1.0 mm ancho / 0.15 mm gap (FR4 1.6 mm, εr 4.3 — User Manual FCC doc 5937666). Matching: 0 Ω + 2.2 pF + DNI.
- UART y LED debug: poner footprint pero no poblar en producción
