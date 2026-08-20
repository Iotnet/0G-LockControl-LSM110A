# Límites eléctricos del LSM110A

**Fase:** F1 · **Fecha:** 2026-08-20
**Fuentes:** DS R08 **§3.1** (pág. 6) · **§3.2** (pág. 6) · **§3.3** (pág. 7) · **§3.4.1** (pág. 7) · **§3.4.2** (pág. 8) · **§3.4.4** (pág. 11) · **§6.1** (pág. 19) · UM Rev1.4 **§1.3** (pág. 5) · `[SJIT]_LSM1x0A_Sigfox_API_manual_v1.1_240118.pdf` **§2.1** (pág. 5)

> **Regla de esta fase (cierra H-12): donde el datasheet publique un `max`, este documento usa el `max`.** Los `typ` se listan solo como referencia. Un presupuesto de energía con valores typ no es un presupuesto, es un deseo.

---

## 1. Absolute Maximum Ratings (DS §3.1, Tabla 3-1-1, pág. 6)

| Parámetro | Min | Typ | **Max** | Unidad |
|---|---|---|---|---|
| Supply Voltage (VDD) | 0 | 3.3 | **3.9** | V |
| Storage Temperature | −40 | — | +85 | °C |
| Operating Temperature | −30 | — | +85 | °C |
| RF Input Power | — | — | **+0** | dBm |
| ESD | −2 | — | +2 | kV |

Nota literal del DS: *«Stress exceeding of one or more of the limiting values listed under "Absolute Maximum Ratings" may cause permanent damage to the radio module»*.

**ESD ±2 kV es bajo** para un pin de antena en el borde de un producto que se manipula. Es el argumento cuantitativo de `red-rf.md` §2 a favor de poblar la red de ESD del DS §6.1.

---

## 2. Características generales (DS §3.2, Tabla 3-2-1, pág. 6)

Condiciones de la tabla: **T = 25 °C, VDD = 3.3 V typ.**

| Parámetro | Condición | Min | Typ | **Max** | Unidad |
|---|---|---|---|---|---|
| Supply Voltage (VDD) | — | **1.8** | 3.3 | **3.6** | V |
| Corriente | **Sleep (Stop2)** | — | 1.8 | **5** | **µA** |
| Corriente | Receive | — | 5 | — | mA |
| Corriente | **Transmit (RF +21 dBm)** | — | **123** | — | mA |
| Reloj | Transceiver | — | 32 | — | MHz |
| Reloj | MCU RTC | — | 32.768 | — | kHz |

### Los tres números que gobiernan F2

**a) `VDD min = 1.8 V`** — es el criterio de aceptación de GATE 2. Si VDD cae por debajo de 1.8 V durante el pulso de TX, el módulo está fuera de especificación y el comportamiento no está definido.

**b) `Sleep = 5 µA max`** (no 1.8 µA typ). Diferencia sobre la vida de una CR2450 de 620 mAh, contando **solo** el módulo y nada más:

| | Corriente | Vida teórica |
|---|---|---|
| typ | 1.8 µA | 39.3 años |
| **max** | **5 µA** | **14.2 años** |

Ambos números son enormes, lo que dice algo importante: **el módulo no es el que agota la pila.** Lo que la agota es todo lo demás. Ver §6.

**c) `TX = 123 mA @ +21 dBm`** — es el único punto de corriente de TX que el datasheet publica. **No hay dato a +14 dBm**, que es donde probablemente vamos a operar. Medirlo es tarea explícita de GATE 2 (F2).

> **Cuidado con el escalado.** No se puede estimar la corriente a +14 dBm dividiendo: la eficiencia del PA no es lineal y el consumo de la parte digital es fijo. 123 mA a +21 dBm no implica ~25 mA a +14 dBm. **Hay que medirlo.**

---

## 3. Interfaz de I/O (DS §3.3, Tabla 3-3-1, pág. 7)

| Símbolo | Parámetro | Condición | Min | Typ | Max | Unidad |
|---|---|---|---|---|---|---|
| V_IL | I/O input low-level | — | — | — | **0.3 × VDD** | V |
| V_IH | I/O input high-level | 1.8 V < VDD < 3.6 V | **0.7 × VDD** | — | — | V |
| V_hys | input hysteresis | — | — | 200 | — | mV |
| BR | UART baud rate | — | — | **9.6** | — | kbps |

Los umbrales son **proporcionales a VDD**, no absolutos. Con la pila a 2.4 V: V_IL ≤ 0.72 V y V_IH ≥ 1.68 V.

**Consecuencia para F3 — el sensor magnético.** Un DRV5032 alimentado desde la misma VDD cumple sin problema. Pero cualquier divisor resistivo o salida open-drain con pull-up alto en PA1/PA0 tiene que garantizar V_IH ≥ 0.7 × VDD **con la pila descargada**, que es el caso peor.

**Nota:** `BR typ 9.6 kbps` es el baud del firmware de usuario. El **bootloader IAP corre a 115200** (`FW Download Guide §1.1 paso 2`). Dos velocidades distintas en el mismo puerto (UART2 / PA2-PA3).

---

## 4. Potencia de salida vs VDD — **el número que más cambia el diseño**

**DS §3.4.4, Tabla 3-4-4-2 (pág. 11), columna Sigfox 902.2 MHz:**

| VDD (V) | P_out @ 902.2 MHz (dBm, typ) | P_out @ 920.8 MHz |
|---:|---:|---:|
| 1.8 | **16.4** | 16.2 |
| 2.0 | 17.4 | 17.2 |
| 2.3 | 18.6 | 18.4 |
| 2.5 | **19.2** | 19.1 |
| **3.0** | **20.5** | 20.4 |
| 3.3 | 21.3 | 21.2 |
| 3.6 | 21.8 | 21.6 |

**La potencia de salida sigue a la tensión de la pila.** Entre pila nueva (3.0 V → 20.5 dBm) y pila casi agotada (1.8 V → 16.4 dBm) se pierden **4.1 dB** — más de la mitad de la potencia. El presupuesto de enlace de un dispositivo a CR2450 se degrada a lo largo de la vida de la pila, y hay que dimensionarlo con el **caso peor de fin de vida**, no con la pila nueva.

Esto refuerza el criterio de GATE 2: mantener VDD alto durante el pulso no es solo cumplir el `VDD min`, es **conservar alcance**.

Referencia cruzada — límites de la banda RC2 (DS §3.4.1, Tabla 3-4-1-2, pág. 7):

| | Min | Typ | Max |
|---|---|---|---|
| **RC2** | 902.104 | **902.2** | 902.296 MHz |
| RC4 | 920.704 | 920.8 | 920.896 MHz |

Nota literal del DS: *«RC2 902.2 +/-0.096»*. Sensibilidad RX RC2 (§3.4.2, Tabla 3-4-2-2): **−124.5 dBm** a 0.6 kbps, medida a 905.2 MHz. Tolerancia de frecuencia de TX: **±2.5 ppm** a 25 °C.

---

## 5. Reset y alimentación — lo que el DS pide y el EVB no trae

Comparación directa de las dos fuentes. **Esta tabla es el origen de casi todas las diferencias legítimas entre la referencia y nuestro producto.**

| | **DS §6.1** Fig. 6-1-1 (pág. 19)<br>*«typical application»* | **EVB** UM §1.3 hoja 2/3 (pág. 5)<br>*alimentado por USB* | **v0 LockControl** |
|---|---|---|---|
| Desacoplo VDD | **C1 = 10 µF + C2 = 100 nF** | C2 = **DNI**/1608 + C3 = **100 nF**/1005 | **seguir el DS: 10 µF + 100 nF** |
| Pull-up en NRST | **R1 = 100 kΩ a VDD** | ninguno | **poblar** |
| Supervisor de reset | **U2 = «1.8 V Reset_IC»** (RST/GND/VIN/NC) | ninguno | **decisión de F3** |
| Reset manual | — | SW1 táctil + **R1 = 390 Ω** en serie a NRST | opcional |
| BOOT0 | sacado a test point | SW2 SPDT: VCC = *Boot-Load_Mode*, abierto = normal | **jumper o test point** |
| Pull-up en PA1 | — | **R8 = 100 kΩ** + C9 = 100 nF antirrebote | ver §6 |
| SWD | CN1 5 pines: SWDIO/SWCLK/GND/VCC/RESET_N | CN3 idéntico | **replicar los 5 pines** |
| USB-serial → UART2 | — | **U2 = CP2104**, R6/R7 = 22 Ω en serie a PA2/PA3 | 22 Ω opcionales |

### 5.1 Por qué el EVB no lleva supervisor de reset — y por qué nosotros sí

El EVB va por **USB**: 5 V regulados, con flanco de subida limpio y rápido. En ese escenario, un supervisor de 1.8 V no aporta nada, y el fabricante lo omitió.

**Nuestro escenario es el del datasheet, no el del EVB.** Con una CR2450 hay dos situaciones que el EVB nunca ve:

1. **Inserción de la pila:** el flanco de VDD depende de cómo el usuario encaja la pila en el portapilas. Un contacto que rebota puede dejar el MCU arrancando repetidamente por debajo de su tensión mínima de operación.
2. **Fin de vida:** la pila cae lentamente por debajo de 1.8 V. Sin supervisor, el MCU entra en una zona no especificada — y esa zona incluye **escrituras a flash con tensión insuficiente**, que es el modo de fallo que puede corromper la zona de credenciales Sigfox (ver `mapa-memoria.md` §3). No es un reset feo: es pérdida irreversible del dispositivo.

**Esto resuelve H-08 con un argumento, no con una preferencia.** La recomendación a F3 es **poblar** el supervisor. Pero es una decisión de Franco (§5-F3 de la guía), y tiene un coste real que hay que poner sobre la mesa: **un supervisor consume corriente de reposo continua** (típicamente 0.2–1 µA según el modelo), que se suma al presupuesto de §6. Elegir uno de bajo I_Q es parte de la decisión, no un detalle.

> Nota de coherencia documental: la guía §5-F3 describe la recomendación del DS como «R 100 k + reset IC». Es correcto — son **dos** componentes distintos (`R1` y `U2`), y en la Fig. 6-1-1 se ven ambos. El pull-up solo no sustituye al supervisor.

### 5.2 El desacoplo: 100 nF no basta con pila

El EVB trae **solo 100 nF** y deja el footprint del bulk **sin poblar** (`C2 = DNI/1608`). Puede permitírselo porque el USB le da una fuente de baja impedancia.

Con una CR2450 —cuya ESR es de decenas de ohmios y **sube conforme se descarga**— los 100 nF no sostienen un pulso de 123 mA. Por eso `bom-mvp-v0.csv` lleva un condensador de soporte grande, y por eso existe GATE 2. **No copiar el desacoplo del EVB. Seguir el DS §6.1 (10 µF + 100 nF) y añadir encima el condensador de soporte de F2.** Son tres cosas distintas con tres papeles distintos: bulk local del módulo (10 µF), alta frecuencia (100 nF), y reserva de energía para el pulso de TX (el de F2).

---

## 6. Entrega a F2 — el término que domina el presupuesto

**H-05, cuantificado.** El pull-up de la entrada del sensor magnético, con el pin en bajo, es una corriente continua de `VDD / R`:

| Pull-up | I continua @ 3.0 V | Agota 620 mAh en |
|---|---:|---:|
| **10 kΩ** (lo que dice el repo) | **300 µA** | **86 días** |
| 100 kΩ (lo que usa la referencia SJI) | 30 µA | 861 días (2.4 años) |
| **DRV5032 push-pull, sin pull-up** | **0 µA** | — |

Contexto: el módulo entero en Stop2 consume **5 µA max**. Un pull-up de 10 kΩ en bajo consume **60 veces más que el módulo completo**. Es, con diferencia, el término dominante del presupuesto de reposo — y es un error de una sola línea de BOM.

La referencia usa 100 kΩ porque su EVB va por USB y 30 µA le da igual. **Nosotros no podemos copiar ese valor**: es una de las tres desviaciones deliberadas de v0 (trampa 5 de la guía). La solución es eliminar el pull-up, no agrandarlo — de ahí la variante **push-pull** del DRV5032, que F3 debe confirmar.

**Nota importante:** este consumo solo existe **con el pin en bajo**. Depende de la polaridad del sensor y de si el estado de reposo (puerta cerrada, imán presente) deja la salida activa o inactiva. **El caso peor es «imán presente de forma sostenida»**, que es precisamente el estado normal de una puerta cerrada. Por eso el criterio de GATE 2 mide el Δ de corriente **con el imán sostenido**, no con un toque.

### Primera aproximación de la energía por evento (para que F2 la refute con medidas)

`tx_repeat` es un **flag, no un contador** — confirmado literal en el Sigfox API manual §2.1 (pág. 5): *«when 0, sends one Tx. when 1, sends three Tx.»* Esto cierra **H-04**: `APP_SENSORS_TX_REPEATS = 2` en nuestro firmware no significa «2 repeticiones», está fuera del dominio válido `{0,1}`.

Con `tx_repeat = 1` → **3 frames**. Tiempo de aire, calculado:

- 12 bytes = 96 bits de payload, a 600 bps → **0.160 s** por frame de payload
- 3 frames → **0.480 s** de payload en el aire
- Carga a 123 mA (peor caso, +21 dBm, sin contar gaps): **0.0164 mAh**

> ⚠ **Discrepancia con la guía, y no es menor.** La guía §5-F2 estima *«tres frames de 12 bytes a 600 bps ≈ 7–9 s de TX»*. El cálculo da **0.48 s de payload en el aire**. La diferencia es de más de un orden de magnitud.
>
> La cifra de 7–9 s probablemente sea el **tiempo total transcurrido** de la secuencia, incluyendo los retardos entre repeticiones que el protocolo Sigfox impone — pero **durante esos retardos el módulo no está transmitiendo, y por tanto no está consumiendo 123 mA**. Confundir «duración de la secuencia» con «tiempo de TX» sobreestima la energía por evento en ~15×, y también subestima el problema real, que no es la energía total sino **la caída de tensión durante cada pulso**.
>
> A esto hay que sumar el preámbulo, sincronización, ID de dispositivo, autenticación y CRC de cada frame, que el API manual no desglosa temporalmente. **F2 debe medirlo con el osciloscopio, no calcularlo**: la traza de VDD da el número exacto de ambas cosas (duración real de cada pulso y separación entre ellos).

Para dar escala del orden de magnitud, y **solo** como cota superior de referencia (ignorando reposo, autodescarga y el derating por ESR):

- 620 mAh / 0.0164 mAh ≈ **38 000 uplinks** teóricos
- a 6 uplinks/día → ~17 años solo por TX
- al tope de 130 msg/día de `DT-006` → ~291 días

**Conclusión que F2 debe verificar:** con la corriente de fuga controlada, el límite de la CR2450 no es la energía acumulada de los uplinks — es **si la pila puede entregar el pico de 123 mA sin que VDD baje de 1.8 V**, sobre todo con la pila envejecida y su ESR ya alta. Eso es exactamente lo que mide GATE 2, y es la razón por la que la prueba se hace con **pila real y no con fuente de banco**.
