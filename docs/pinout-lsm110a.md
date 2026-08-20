# Asignación de pines — LSM110A en 0G LockControl

> **Este documento ya no es la fuente de verdad del pinout.**
>
> La tabla completa de los 34 pines, derivada de la **Tabla 5-1-1 del `DS_LSM110A_R08`**
> (págs. 14–15) y verificada contra 3 fuentes independientes, está en
> **[`Hardware/v0-replica-sji/00-fuente-de-verdad/pinout-34-pines.md`](../Hardware/v0-replica-sji/00-fuente-de-verdad/pinout-34-pines.md) §1**.
>
> La versión anterior de este archivo tenía **4 pines mal** (hallazgo H-01) y nunca se
> derivó de la Tabla 5-1-1, así que se reemplazó entera en lugar de parchearse
> (decisión **D-04**). Lo que queda abajo es solo el subconjunto que usa v0, derivado de
> esa tabla. Si los dos documentos discrepan, manda `pinout-34-pines.md`.

## Pines usados en v0

| Pin | GPIO | Función | Configuración | Notas |
|----:|------|---------|---------------|-------|
| 11 | VDD | Alimentación | 3.0V directo o 2.5V LDO | 10 µF + 100 nF (DS §6.1) |
| 1, 10, 12, 20, 23, 32, 34 | GND | Tierra | Plano GND | **Los 7 hay que conectarlos** |
| 7 | PA13 | SWDIO | Debug SWD | Header 4 pines |
| 8 | PA14 | SWCLK | Debug SWD | Header 4 pines |
| 3 | PA9 | I2C1_SCL | Open-drain + pull-up | Acelerómetro LIS2DW12 |
| 4 | PA10 | I2C1_SDA | Open-drain + pull-up | Acelerómetro LIS2DW12 |
| 16 | PA0 | EXTI wake-up | Input, pull-down | INT1 del LIS2DW12 |
| 15 | PA1 | Wake-up | Input (tipo `I` en la tabla) | Reed switch o DRV5032 |
| 14 | PA2 | **UART2_TX** | — | **Puerto IAP — no usar para otra cosa** |
| 13 | PA3 | **UART2_RX** | — | **Puerto IAP — no usar para otra cosa** |
| 19 | PB6 | UART1_TX | Output (debug) | No poblar en producción |
| 18 | PB7 | UART1_RX | Input (debug) | No poblar en producción |
| 30 | NRST | Reset | Ver `limites-electricos.md` §5 | Supervisor 1.8 V: decisión de F3 |
| 31 | BOOT0 | Boot mode | Pull-down interno 10 kΩ | Test point / jumper |
| 33 | RFOUT | Salida RF 50Ω | Traza CPWG (1.0 mm / gap 0.15 mm) | Antena PCB diseño SJI |

## Notas de diseño

- **El LED de debug no va en PA2.** PA2 es `UART2_TX`, la única vía de rescate por IAP
  si el firmware deja el módulo sin SWD. El diseño de referencia de SJI pone sus LEDs en
  **PA8 (pin 24), PA11 (pin 5) y PA15 (pin 9)** — los tres libres para nosotros y ninguno
  choca con UART2. La elección la cierra F3 (hallazgo H-02).
- **UART2 (pines 13 y 14) es obligatorio**, no opcional: es el puerto del bootloader.
  Ver `00-fuente-de-verdad/mapa-memoria.md` §4.
- Los pines **26 y 27 no son I2C.** Son `PB5` (SPI1_MOSI) y `PA6` (SPI1_MISO). La versión
  anterior de este documento les atribuía SCL/SDA; ese era uno de los 4 errores de H-01.
- Pull-ups de I2C: el repo dice 4.7 kΩ y la BOM dice 10 kΩ. **Sin resolver** — decidir uno
  y alinear ambos documentos (pregunta abierta de F3 en `ESTADO.md`).
- Todos los pines GND deben conectarse al plano de tierra sólido.
- Capacitores de desacoplo lo más cerca posible del pin VDD.
- Traza RF: CPWG 50 Ω, 1.0 mm ancho / 0.15 mm gap (FR4 1.6 mm, εr 4.3 — User Manual FCC
  doc 5937666). Matching: 0 Ω + 2.2 pF + DNI. El recálculo con el stackup real de JLCPCB
  sigue pendiente (deuda **B-03**).
- UART y LED debug: poner footprint pero no poblar en producción.
