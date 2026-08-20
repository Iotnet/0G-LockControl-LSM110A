# Pinout LSM110A — 34 pines, re-derivado de cero

**Fase:** F1 · **Fecha:** 2026-08-20
**Fuente primaria:** `DS_LSM110A_R08_241008.pdf`, **Tabla 5-1-1** (págs. 14–15)
**Verificación cruzada 1:** DS R08 **Fig. 5-1-1** (pág. 14) — símbolo de esquemático con funciones del diseño de referencia
**Verificación cruzada 2:** `[SJIT]_LSM110A_UserManual_Rev1.4_240626.pdf` **§1.4** (pág. 7) — columna *Module Pin No.*
**Verificación cruzada 3:** UM **§1.3** hoja 2/3 (pág. 5) — bloque `MODULE_OUT_INTERFACE`, y DS **§6.1** Fig. 6-1-1 (pág. 19)

> **No se copió ninguna fila de `docs/pinout-lsm110a.md`.** Ese documento tiene 4 errores confirmados (§Discrepancias).

---

## 1. Tabla completa (las 34 filas)

Columnas:
- **Pin / Nombre / Tipo / MCU / Descripción** → literal de la Tabla 5-1-1 (DS R08 págs. 14–15).
- **Etiqueta ref. SJI** → nombre que le da el diseño de referencia en Fig. 5-1-1 y en el esquemático del EVB. Es el uso de SJI, no una función del silicio.
- **v0 LockControl** → qué le conectamos nosotros. `—` = sin conexión decidida todavía (lo cierra F3).

| Pin | Nombre | Tipo | MCU | Descripción (DS Tabla 5-1-1) | Etiqueta ref. SJI | v0 LockControl |
|----:|--------|:----:|-----|------------------------------|-------------------|----------------|
| 1  | GND   | —   | —     | Ground | GND | Plano GND |
| 2  | PB2   | I/O | PB2   | General purpose IO, selectable ADC functionality | PB2(ADC_IN4) | — (libre) |
| 3  | PA9   | I/O | PA9   | General purpose IO, selectable **I2C(SCL)** functionality | PA9(I2C1-SCL) | **I2C1_SCL → LIS2DW12** |
| 4  | PA10  | I/O | PA10  | General purpose IO, selectable **I2C(SDA)** functionality | PA10(I2C1-SDA) | **I2C1_SDA → LIS2DW12** |
| 5  | PA11  | I/O | PA11  | General purpose IO, selectable I2C2(SDA) functionality | PA11(LED2) | — (candidato LED) |
| 6  | PA12  | I/O | PA12  | General purpose IO, selectable I2C2(SCL) functionality | PA12(RF-BUSY) | — |
| 7  | PA13  | I/O | PA13  | **Serial-Wire Debug Data** (FW Down-load) | PA13(SWDIO) | **SWDIO** |
| 8  | PA14  | I/O | PA14  | **Serial-Wire Debug Clock** (FW Down-load) | PA14(SWCLK) | **SWCLK** |
| 9  | PA15  | I/O | PA15  | General purpose IO | PA15(LED3) | — (candidato LED) |
| 10 | GND   | —   | —     | Ground | GND | Plano GND |
| 11 | VDD   | P   | —     | **Power Supply (+1.8 V ~ +3.6 V)** | VCC | **VDD** (10 µF + 100 nF, DS §6.1) |
| 12 | GND   | —   | —     | Ground | GND | Plano GND |
| 13 | PA3   | I/O | PA3   | **USART2 RX Data** | PA3(UART2-RX) | **UART2_RX — puerto IAP (obligatorio)** |
| 14 | PA2   | I/O | PA2   | **USART2 TX Data** | PA2(UART2-TX) | **UART2_TX — puerto IAP (obligatorio)** |
| 15 | PA1   | **I** | PA1 | **Wake-up**, General purpose IO | PA1(BUT2) | **Sensor magnético (wake-up)** |
| 16 | PA0   | I/O | PA0   | General purpose IO | PA0(BUT1) | **INT1 del LIS2DW12** |
| 17 | PB8   | I/O | PB8   | General purpose IO | PB8(RF-IRQ2) | — (libre) |
| 18 | PB7   | I/O | PB7   | USART1 RX Data | PB7(UART1-RX) | UART1_RX (debug, no poblar) |
| 19 | PB6   | I/O | PB6   | USART1 TX Data | PB6(UART1-TX) | UART1_TX (debug, no poblar) |
| 20 | GND   | —   | —     | Ground | GND | Plano GND |
| 21 | PB3   | I/O | PB3   | General purpose IO | PB3 | — (candidato LED) |
| 22 | PB4   | I/O | PB4   | General purpose IO | PB4 | — (candidato LED) |
| 23 | GND   | G   | —     | Ground | GND | Plano GND |
| 24 | PA8   | I/O | PA8   | General purpose IO | PA8(LED1) | — (candidato LED) |
| 25 | PA7   | I/O | PA7   | General purpose IO | PA7(DBG4) | — (libre) |
| 26 | PB5   | I/O | PB5   | General purpose IO, selectable SPI1 MOSI functionality | PB5(SPI1-MOSI) | — (libre) |
| 27 | PA6   | I/O | PA6   | General purpose IO, selectable SPI1 MISO functionality | PA6(SPI1-MISO) | — (libre) |
| 28 | PA5   | I/O | PA5   | General purpose IO, selectable SPI1 SCK functionality | PA5(SPI1-SCK) | — (libre) |
| 29 | PA4   | I/O | PA4   | General purpose IO, selectable SPI1 NSS functionality | PA4(SPI1-NSS) | — (libre) |
| 30 | NRST  | I/O | NRST  | **IC Reset** | NRST | **NRST** (ver `limites-electricos.md` §5) |
| 31 | BOOT  | I/O | BOOT0 | **IC BOOT0 (Internal pull-down 10 kΩ resistor)** | BOOT | **BOOT0** (test point / jumper) |
| 32 | GND   | —   | —     | Ground | GND | Plano GND |
| 33 | RFOUT | **A** | — | **RF input/output** | RFOUT | **RF → red de matching → antena** |
| 34 | GND   | —   | —     | Ground | GND | Plano GND |

**Conteos verificados:** 34 filas · **7 pines GND** = 1, 10, 12, 20, 23, 32, 34 · 1 VDD (11) · 1 RF (33) · 2 de sistema (NRST 30, BOOT 31) · 23 GPIO.

---

## 2. Distribución física (DS Fig. 5-1-1 y Fig. 5-3-1, págs. 14 y 16)

```
                 ┌─────────────────────────┐
      pin  1 ───►│ o                     34│◄─── pin 34
             ... │                         │ ...
      pin 12 ───►│                       23│◄─── pin 23
                 └──┬┬┬┬┬┬┬┬┬┬─────────────┘
                    13 14 ... 22
```

- **Columna izquierda:** pines **1…12** (12 pads), 1 arriba → 12 abajo. Marca de pin 1 = círculo.
- **Fila inferior:** pines **13…22** (10 pads), 13 a la izquierda → 22 a la derecha.
- **Columna derecha:** pines **23…34** (12 pads), 23 abajo → 34 arriba.
- **Borde superior: sin pads. No hay pad central.** La Tabla 5-1-1 no lista ninguno.

---

## 3. Trampa del User Manual §1.4 (por qué H-01 existe)

La tabla del UM §1.4 (pág. 7) tiene **dos columnas de número de pin**:

| Conector | *Pin No.* (del header del EVB) | *Module Pin No.* (del módulo) |
|---|---|---|
| **J1** | 1…16 | coinciden casi 1:1 con el módulo (1→2, 3→3, … 15→15) |
| **J2** | 1…16 | **NO coinciden**: J2-1 → módulo 31, J2-2 → 30, J2-3 → 29 … J2-15 → 16 |

En J1 las dos columnas casi coinciden, así que confundirlas no se nota. **En J2 están invertidas** y confundirlas produce errores grandes. Para números de pin del módulo, la única fuente es la **Tabla 5-1-1 del DS R08**.

Comprobación de J2 usada aquí (UM §1.4, pág. 7): `BOOT→31 · NRST→30 · PA4→29 · PA5→28 · PA6→27 · PB5→26 · PA7→25 · PA8→24 · PB4→22 · PB3→21 · GND→20 · PB6→19 · PB7→18 · PB8→17 · PA0→16`. Coincide fila por fila con la Tabla 5-1-1. ✅

---

## 4. Discrepancias detectadas

### H-01 — `docs/pinout-lsm110a.md` tiene 4 pines mal · **CONFIRMADO**

| Señal | Repo dice | Correcto (DS Tabla 5-1-1) | Qué hay realmente en el pin que el repo cita |
|---|:---:|:---:|---|
| PA9 (I2C1_SCL)  | 26 | **3**  | pin 26 = **PB5** (SPI1-MOSI) |
| PA10 (I2C1_SDA) | 27 | **4**  | pin 27 = **PA6** (SPI1-MISO) |
| PA0 (INT1 accel)| 14 | **16** | pin 14 = **PA2** (USART2_TX) |
| PA2 (LED debug) | 16 | **14** | pin 16 = **PA0** |

Los dos últimos son un **intercambio PA0↔PA2**. Consecuencia práctica: si el layout se rutea por número de pin y el firmware por nombre de señal, el LED de debug aterriza sobre `USART2_TX` (el puerto del bootloader IAP) o sobre `INT1` del acelerómetro. **Cualquiera de las dos rompe el rescate por IAP o la interrupción del acelerómetro.** → alimenta **H-02**.

**Acción:** `docs/pinout-lsm110a.md` debe reemplazarse por la tabla de §1 de este documento. No corregir 4 celdas: el documento nunca se derivó de la Tabla 5-1-1 y hay que rehacerlo.

### H-13 — El MCU no es STM32WL55 · **RESUELTO**

**DS R08 §1.1 «Key Features» (pág. 4), última viñeta, literal: `- STM32WLE5CC`.**

Es la única mención de un número de parte de MCU en los 5 documentos del fabricante (verificado por búsqueda sobre los 11 PDFs del repo `Support-SJI/LSM110A`: 1 sola coincidencia, `STM32WLE5CC`).

El repo dice otra cosa en 20 sitios (`STM32WL55JCIX` ×11, `STM32WL55JC` ×4, `STM32WL55` ×4, `STM32WL55JCI7U` ×2, …). Ese es el MCU de la **placa Nucleo-WL55JC** que se usó en el prototipo previo; se arrastró al repo del módulo.

**Importa por dos razones, no una:**
1. `STM32WL55JC` es **dual-core** (Cortex-M4 + M0+). `STM32WLE5CC` es **single-core M4**. La guía §3 dice «STM32WL55JC (single-core)» — está mal en ambos términos.
2. Flash y RAM difieren, y el mapa de memoria de `mapa-memoria.md` (que llega a `0x0803E5xx`, o sea 250 kB) solo cierra con un dispositivo de **256 kB** — que es lo que la letra `C` de `WLE5CC` designa. Confirma indirectamente el WLE5CC.

**Acción F1:** corregir todas las referencias del repo a **STM32WLE5CC**. Nota para F7: el `STM32CubeProgrammer` reportará la familia **STM32WLE5**, que es lo que el checklist de bring-up debe esperar.

### Nota de contexto — otras etiquetas de la referencia que NO son función del silicio

La Fig. 5-1-1 y el esquemático del EVB etiquetan `PA8(LED1)`, `PA11(LED2)`, `PA15(LED3)`, `PA12(RF-BUSY)`, `PA7(DBG4)`, `PB8(RF-IRQ2)`, `PA0(BUT1)`, `PA1(BUT2)`. **Son usos del EVB de SJI, no funciones del módulo.** La Tabla 5-1-1 lista todos esos pines como *General purpose IO*. Dato útil para F3: el diseño de referencia pone sus LEDs en **PA8 (24), PA11 (5) y PA15 (9)** — ninguno choca con UART2, y los tres son pines que nosotros tenemos libres.

---

## 5. Cómo verificar cualquier fila de este documento

1. Abrir `pdfs/DS_LSM110A_R08_241008.pdf` en la **pág. 14** (pines 1–7) o la **pág. 15** (pines 8–34).
2. Los PDFs traen la Tabla 5-1-1 como **texto**, no como imagen: `pdftotext -f 14 -l 16 -layout` la devuelve completa. (La Fig. 5-1-1 sí es imagen y hay que renderizarla.)
3. Para la verificación cruzada, `pdfs/[SJIT]_LSM110A_UserManual_Rev1.4_240626.pdf` **pág. 7**, columna *Module Pin No.* — nunca la columna *Pin No.*
