# Selección de acelerómetro — 0G LockControl (LSM110A)

**Empresa:** 0G IoT Solutions — <https://0giotsolutions.com/>
**Fecha:** 2026-09-01
**Estado:** análisis de alternativas + plan de abastecimiento. Confirma `U2 = LIS2DW12` y define segunda fuente.
**Relacionado:** DT-017 en [`decisiones-tecnicas.md`](./decisiones-tecnicas.md) · [`spec-producto.md`](./spec-producto.md) §3 · [`../Hardware/BOM/bom-mvp-v0.csv`](../Hardware/BOM/bom-mvp-v0.csv) fila U2

---

## 1. Por qué este documento

El repo ya nombra el **LIS2DW12** en la BOM, el esquemático, el pinout y el driver
(`Firmware/drivers/lis2dw12.c`, hitos Y1–Y3 cerrados). Este documento no reabre esa decisión
a ciegas: **verifica** que sigue siendo la correcta contra las alternativas reales del mercado,
deja documentada la segunda fuente, y resuelve el problema práctico de **dónde se compra**
—que no es el mismo lugar para el chip de producción que para la pieza de banco.

## 2. El filtro: qué exige el diseño que ya existe

Cualquier candidato tiene que pasar estos seis requisitos. Salen del hardware y del firmware
ya congelados, no de una lista genérica de deseos.

| # | Requisito | Valor | De dónde sale |
|---|---|---|---|
| R1 | **Consumo en reposo con detección activa** | **≤ 1 µA** | Presupuesto de sleep del producto: 5 µA max (DT-013). El LSM110A en Stop2 y el DRV5032 ya se comen la mayor parte |
| R2 | **Tensión de alimentación** | **1.7 – 3.6 V**, funcionando a 2.0 V | CR2450 directa: 3.0 V nueva → ~2.0 V a fin de vida (DT-003, config `U4b` bypass 0 Ω) |
| R3 | **Interfaz** | **I²C** | PA9/PA10 son los únicos pines cableados al bus. Un sensor SPI obligaría a re-rutear y a gastar 2 GPIO más |
| R4 | **Interrupción de hardware por umbral** | 1 pin INT, push-pull, activo alto, con umbral programable en mg | El diseño es *event-driven*: el chip debe despertar al MCU por sí solo (PA0 / EXTI0, pull-down). Sin esto no hay arquitectura |
| R5 | **Motion engine con eje dominante** | registro tipo `WAKE_UP_SRC` | El payload Sigfox gasta el byte 4 en «eje dominante» y los bytes 2–3 en magnitud. Si el chip no lo reporta, lo tiene que calcular el MCU despierto → más consumo |
| R6 | **Encapsulado y costo** | ≤ 2×2 mm, ensamblable en JLCPCB, < 1 USD @1k | Objetivo de caja < 5 cm y costo de manufactura |

> **Nota sobre R1 y la política DT-013.** Los fabricantes de MEMS publican consumo *typical*,
> casi nunca *max*. Donde no haya `max` publicado, el presupuesto debe aplicar margen sobre el
> `typ` en lugar de tomarlo tal cual. Esto es deuda abierta para el GATE 2.

## 3. Candidatos evaluados

### 3.1 Comparativa

| Parte | Fabricante | Encapsulado | VDD | Consumo con detección activa | Interfaz | Motion engine | R1–R6 |
|---|---|---|---|---|---|---|---|
| **LIS2DW12** | ST | LGA-12 2×2×0.7 | 1.62–3.6 V | 50 nA power-down · **0.38 µA @1.6 Hz** (LP mode 1) · sub-µA @12.5 Hz | I²C + SPI, 2 INT | wake-up, free-fall, 6D/4D, tap/double-tap, activity/inactivity, stationary/motion | ✅ todos |
| **LIS2DTW12** | ST | LGA-12, **pin-compatible** con la familia 2×2 | igual | equivalente | I²C + SPI | el mismo + **termómetro absoluto ±0.8 °C typ (0–70 °C), 12 bits** | ✅ todos |
| **LIS2DH12** | ST | LGA-12, pin-compatible | 1.71–3.6 V | ~2 µA en low-power | I²C + SPI | wake-up, free-fall, 6D/4D, click | ⚠️ R1 justo |
| **BMA400** | Bosch | LGA-12 2×2×0.95 (**pinout distinto**) | 1.72–3.6 V *(confirmar en DS)* | 200 nA sleep · **800 nA** ultra-low-power · 3.5 µA caso de uso LP | I²C + SPI | **auto-wakeup / auto-low-power**, activity/inactivity, step counter, orientación, tap/double-tap | ✅ salvo footprint |
| **ADXL362** | Analog Devices | LGA-16 3×3.25×1.06 | 1.6–3.5 V | **270 nA** en wake-up por movimiento · <2 µA @100 Hz | ❌ **solo SPI** | activity/inactivity con umbral, FIFO, sensor de temp | ❌ **R3** |
| **ADXL345** | Analog Devices | LGA-14 3×5×1 | 2.0–3.6 V | 0.1 µA standby · **23–40 µA midiendo** | I²C + SPI | activity/inactivity, tap/double-tap, free-fall | ❌ **R1** (~30×) |
| **ADXL335** (GY-61) | Analog Devices | — | 1.8–3.6 V | ~320 µA, **salida analógica** | ADC | ❌ ninguno | ❌ **R1, R4, R5** |
| **MPU6050 / BMI160 / BMI270 / BNO085 / ICM-20948** | varios | — | — | mA en operación completa | I²C/SPI | (IMU 6–9 DoF) | ❌ **R1** por 2–3 órdenes de magnitud |

### 3.2 Qué significa eso en vida de batería

CR2450 = 620 mAh. En este producto el consumo **en reposo domina**: transmite unas pocas veces
al día, así que la corriente de sleep es la que fija la vida útil.

| Acelerómetro | Corriente de sleep del sistema (aprox.) | Vida teórica solo por consumo |
|---|---|---|
| LIS2DW12 (~1 µA) | ~5 µA (presupuesto DT-013) | ≈ 124 000 h ≈ **14 años** → la limita la autodescarga de la pila, no el circuito |
| ADXL345 (~30 µA) | ~34 µA | ≈ 18 000 h ≈ **2 años**, y eso *antes* de contar TX y autodescarga |
| MPU6050 (6 ejes, ~3.9 mA) | ~3.9 mA | ≈ 159 h ≈ **6.6 días** |

La conclusión no es matizada: **un IMU de 6/9 ejes convierte el producto en desechable a la
semana.** El giroscopio no aporta nada aquí —detectar que una puerta se movió no necesita
velocidad angular— y es exactamente lo que se come la pila.

### 3.3 Por qué se descarta el ADXL362 pese a ser el de menor consumo

270 nA en modo wake-up es mejor que cualquier ST de la tabla. Se descarta igual porque **es
solo SPI**: obligaría a re-rutear PA9/PA10, gastar dos GPIO adicionales (CS y SCK/MISO/MOSI),
rehacer el esquemático y el layout ya cotejados, y tirar el driver `lis2dw12.c` completo. Se
gana ~0.7 µA de un presupuesto de 5 µA a cambio de reabrir hardware y firmware. No compensa.
Queda anotado como **plan de contingencia** si el GATE 2 revela que el presupuesto de energía
no cierra por un margen grande.

## 4. Recomendación

### 4.1 Producción — **mantener LIS2DW12TR** (sin cambio)

Es el único de la comparativa que cumple **R1–R6 a la vez**, y además:

- **Ya está integrado.** Driver escrito y validado en la NUCLEO (`WHO_AM_I = 0x44`, wake-up a
  200 mg, decodificación de eje por `WAKE_UP_SRC`, hitos Y1–Y3 cerrados). Footprint LGA-12,
  pinout PA9/PA10/PA0 y BOM ya cotejados.
- **Precio y stock verificados:** $0.77 @100u / $0.71 @1k, ~29 551 pzas en LCSC (C189624),
  con ruta directa a ensamble JLCPCB.
- **Su motion engine cubre el payload completo:** umbral en mg (`WAKE_UP_THS`), filtro de
  duración (`WAKE_UP_DUR`), eje dominante y magnitud, más `SLEEP_ON` para que el propio chip
  baje a 12.5 Hz solo. Todo eso es trabajo que el MCU no hace despierto.

Cambiarlo hoy costaría re-hacer M1/M2 sin ganancia técnica medible.

### 4.2 Segunda fuente drop-in — **LIS2DTW12**, y **LIS2DH12** como respaldo barato

- **LIS2DTW12** es **pin-compatible** con la familia 2×2 (LIS2DW12 / LIS2DH12 / LIS2DE12): entra
  en el mismo footprint sin tocar el layout. Trae el mismo motion engine **más un termómetro
  absoluto de ±0.8 °C typ (0–70 °C) a 12 bits**. Eso es directamente relevante aquí: el
  **byte 7 del payload es temperatura**, y hoy se lee del sensor interno del LIS2DW12, que está
  pensado para compensar offset del acelerómetro, no para medir temperatura absoluta. Para el
  caso de uso «cuarto frío» de la spec, esa diferencia es la que separa un dato publicable de
  uno decorativo.
  → **Pendiente de verificar antes de comprometerlo:** `WHO_AM_I` y el mapa de registros de
  temperatura difieren; el driver se reutiliza casi entero pero **no es idéntico**.
- **LIS2DH12** ya figura como sustituto en la BOM. Es más barato y pin-compatible, pero
  ~2 µA (roza R1) y **su mapa de registros no es el mismo** que el del LIS2DW12: implica
  trabajo de driver. Respaldo por quiebre de stock, no elección técnica.

### 4.3 Alternativa técnica real (no drop-in) — **BMA400**

El único competidor que le gana al LIS2DW12 en consumo *sin* romper R3: **800 nA** en modo
ultra-low-power, 200 nA en sleep, I²C, LGA-12 de 2×2. Su ventaja diferencial es el
**auto-wakeup / auto-low-power**: el chip decide solo cuándo subir el ODR, sin intervención
del MCU. Bosch lo posiciona explícitamente para cerraduras inteligentes y sensores de
puerta/ventana, que es literalmente este producto.

**El pinout no coincide** con la familia 2×2 de ST → exige footprint nuevo y respin. Por eso
es plan B para una revisión futura (v1), no un swap de BOM.

### 4.4 Descartados

ADXL362 (§3.3) · ADXL345, ADXL335 y todos los IMU 6/9 DoF (§3.2) · todo el catálogo actual de
UNIT y AG para **producción**.

## 5. Abastecimiento — dónde se compra cada cosa

Aquí hay que ser explícito, porque el resultado de la búsqueda es incómodo:

> **Ni UNIT Electronics ni AG Electrónica venden el LIS2DW12, ni ningún MEMS sub-µA en
> LGA-12 en carrete.** Son tiendas orientadas a maker: su catálogo de movimiento son placas
> breakout. Verificado en ambos sitios.

Lo que **sí** tienen, y qué es cada cosa:

| Tienda | Producto | Qué es | ¿Sirve? |
|---|---|---|---|
| **UNIT Electronics** | [Acelerómetro ADXL345 GY-291](https://uelectronics.com/producto/acelerometro-adxl345-gy-291/) — $69–75 MXN | ADXL345 en breakout, I²C/SPI, 2 pines INT | ✅ **para banco** · ❌ producción (R1) |
| UNIT Electronics | [BMI160](https://uelectronics.com/producto/bmi160-sensor-giroscopio-y-acelerometro-de-6dof/) $43 · [BMI270](https://uelectronics.com/producto/bmi270-sensor-inercial-imu-6dof-i2c-spi-unit-devlab/) $90 · [GY-87 10DoF](https://uelectronics.com/producto/gy-87-modulo-sensor-imu-10dof/) $157 · [MPU-9250](https://uelectronics.com/producto/mpu-9250-imu-de-9dof-9250/) · [MPU6050](https://uelectronics.com/producto/imu-mpu6050-6-grados-de-libertad/) · [BNO085](https://uelectronics.com/producto/gy-bno085-sensor-imu-de-9dof/) | IMU 6–9 DoF | ❌ el giroscopio no se usa y funde la CR2450 |
| **AG Electrónica** | [OKY3246](https://www.agelectronica.com/detalle?busca=OKY3246) — ADXL335 / GY-61, $160.34 MXN + IVA | Acelerómetro **analógico** de 3 ejes | ❌ sin interrupción → rompe la arquitectura event-driven (R4) |

### 5.1 Plan de compra

**(a) Chip de producción — `U2`, el que va soldado en la PCB**

| Canal | Referencia | Nota |
|---|---|---|
| **LCSC** (recomendado) | `LIS2DW12TR` — C189624 | Ya en la BOM. Es el único que integra con el ensamble JLCPCB del mismo pedido |
| DigiKey México | [LIS2DW12TR](https://www.digikey.com.mx/en/products/detail/stmicroelectronics/LIS2DW12TR/7348326) | Canal con entrega y facturación a México, 1–5 días |
| Arrow México | [LIS2DW12TR](https://www.arrow.com/es-mx/products/lis2dw12tr/stmicroelectronics) | Alternativa autorizada |
| ST eStore | [LIS2DW12TR](https://estore.st.com/en/lis2dw12tr-cpn.html) | Cantidades chicas directo de fábrica |

**(b) Pieza de banco — para cerrar Y4 (M2) y M4 en la NUCLEO-WL55JC, ahora**

1. **En UNIT, hoy: ADXL345 GY-291 (~$70 MXN).** Valida *exactamente* la arquitectura que
   importa: I²C + INT push-pull hacia un pin EXTI, umbral en mg, activity/inactivity,
   interrupción con latch y `WHO_AM_I`. Sirve para probar la cadena
   `ISR → flag → task del scheduler → payload → TX Sigfox` sin esperar importación.
   **Lo que NO valida: el consumo.** El GY-291 trae LDO y level shifters a bordo que por sí
   solos superan el presupuesto del producto. Para medir corriente hay que usar el chip real.
2. **Para medir consumo de verdad — dos opciones, ambas de canal formal:**
   - **STEVAL-MKI179V1** (ST): placa adaptadora oficial del LIS2DW12 con los desacoplos ya
     puestos, en DIL24. Es la ruta limpia. Se consigue en DigiKey MX / Mouser / ST eStore.
   - **Comprar 10–20 pzas de LIS2DW12TR** (a $0.77 c/u el costo es irrelevante) y montarlas en
     un breakout LGA-12 propio. Aprovecha el pedido de PCB de M5.

> **Sobre «tienda nacional»:** para el chip desnudo no existe mostrador mexicano que lo tenga
> en carrete. El canal nacional real es **DigiKey México / Mouser / Arrow es-mx**, que facturan
> y entregan en México. Se revisó también SSDIELECT (aparece en búsquedas con módulos LIS3DH):
> **es de Colombia, no de México** — descartado como opción nacional.

## 6. Impacto en el trabajo ya hecho

| Área | Impacto de esta recomendación |
|---|---|
| Esquemático / layout | **Ninguno.** `U2` no cambia; footprint LGA-12 y pines PA9/PA10/PA0 se mantienen |
| BOM | **Ninguno** en la fila U2. Se añade `LIS2DTW12` como segunda fuente pin-compatible junto al `LIS2DH12` ya listado |
| Firmware | **Ninguno** en la ruta principal. `lis2dw12.c` sigue siendo el driver de producción |
| Banco | Compra menor en UNIT (ADXL345 GY-291) para no bloquear Y4 esperando importación |

## 7. Pendientes

- [ ] Confirmar VDD y pinout del **BMA400** contra su datasheet antes de considerarlo para v1
- [ ] Verificar `WHO_AM_I` y registros de temperatura del **LIS2DTW12** antes de comprometerlo
      como segunda fuente activa
- [ ] Leer la Tabla 12 del DS del LIS2DW12 y fijar el consumo exacto a 12.5 Hz en LP mode 1,
      con el margen que exige DT-013 (el fabricante no publica `max`)
- [ ] Decidir en GATE 2 si el termómetro absoluto del LIS2DTW12 justifica el cambio de parte
      para el caso de uso «cuarto frío»

## 8. Fuentes

- [LIS2DW12 — STMicroelectronics](https://www.st.com/en/mems-and-sensors/lis2dw12.html) · [datasheet](https://www.st.com/resource/en/datasheet/lis2dw12.pdf) · [AN5038](https://www.st.com/resource/en/application_note/an5038-lis2dw12-alwayson-3axis-accelerometer-stmicroelectronics.pdf)
- [LIS2DTW12 — STMicroelectronics](https://www.st.com/en/mems-and-sensors/lis2dtw12.html) · [datasheet](https://www.st.com/resource/en/datasheet/lis2dtw12.pdf) · [nota de Future Electronics sobre compatibilidad de pines](https://www.futureelectronics.com/resources/ftm/intelligent-sensing-and-signal-chain/stmicroelectronics-lis2dtw12)
- [LIS2DH12 — STMicroelectronics](https://www.st.com/en/mems-and-sensors/lis2dh12.html)
- [BMA400 — Bosch Sensortec](https://www.bosch-sensortec.com/en/products/motion-sensors/accelerometers/bma400) · [datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bma400-ds000.pdf)
- [ADXL362 — Analog Devices](https://www.analog.com/en/products/adxl362.html) · [datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adxl362.pdf)
- [ADXL345 — Analog Devices](https://www.analog.com/en/products/adxl345.html) · [datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adxl345.pdf)
- [STEVAL-MKI179V1 — adaptador LIS2DW12](https://www.st.com/en/evaluation-tools/steval-mki179v1.html)
- [UNIT Electronics — sensores de movimiento](https://uelectronics.com/categoria-producto/sensores/movimiento/) · [ADXL345 GY-291](https://uelectronics.com/producto/acelerometro-adxl345-gy-291/)
- [AG Electrónica — OKY3246 (ADXL335 GY-61)](https://www.agelectronica.com/detalle?busca=OKY3246)
- [LCSC — LIS2DW12TR C189624](https://www.lcsc.com/product-detail/C189624.html) · [DigiKey México](https://www.digikey.com.mx/en/products/detail/stmicroelectronics/LIS2DW12TR/7348326) · [Arrow México](https://www.arrow.com/es-mx/products/lis2dw12tr/stmicroelectronics)
