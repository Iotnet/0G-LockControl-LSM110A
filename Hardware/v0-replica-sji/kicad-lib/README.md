# Bibliotecas KiCad de 0G LockControl

**Casa única de las bibliotecas del proyecto.** Todo lo que se coloca en una placa de
0G LockControl sale de esta carpeta: el módulo, la antena y los componentes de RF. Un solo
path registrado en KiCad, un solo nickname de footprints.

## Contenido

| Archivo | Qué es | Origen | Fuente |
|---|---|---|---|
| `LSM110A.kicad_sym` | Símbolo, 34 pines | F1, por script | DS R08 **Tabla 5-1-1**, págs. 14–15 |
| `LSM110A.kicad_mod` | Footprint LGA-34 | F1, por script | DS R08 **Fig. 5-3-1** y **Fig. 5-2-1**, pág. 16 |
| `0G_Antenna.kicad_sym` | Símbolo de la antena, 2 pines (FEED, GND) | **generado** | `antenna_geometry.py` |
| `ANT_IFA_915MHz_LSM110A.kicad_mod` | La antena: cobre + keepout + documentación | **generado** | UM §1.5 + arte FCC pág. 5 |
| `ANT_LSM110A_BreakAwaySlots_EVM_ONLY.kicad_mod` | Troquelado del EVM — **NO en el producto** | **generado** | UM §1.9 (medido, ±0.12 mm) |
| `C_0402_1005Metric_0G.kicad_mod` | Land 0402 para L101 / C101 / C102 | **generado** | red de matching, `red-rf.md` |
| `TP_Coax_50R_NanoVNA.kicad_mod` | Isla de 3 pads para pigtail coaxial | **generado** | medida en conducido |

> **Los seis marcados «generado» NO se editan a mano.** Salen de
> `../../KiCad/tools/build.sh`, que los reescribe desde `antenna_geometry.py` y después los
> verifica con el motor de KiCad (**142 comprobaciones, DRC = 0**). Cualquier cambio manual
> se pierde en el siguiente build. Para mover una cota, se corrige `antenna_geometry.py`.
>
> Los dos del LSM110A **no** los toca ese build: son de F1 y se quedan como están.

---

## Instalar en KiCad

**Footprints** — *Preferences → Manage Footprint Libraries → Project Specific → `+`*

| Campo | Valor |
|---|---|
| Nickname | `0G-LockControl` |
| Library Path | `${KIPRJMOD}/../Hardware/v0-replica-sji/kicad-lib` |
| Format | KiCad |

Con esa **única** entrada aparecen los cinco footprints: `LSM110A`, `ANT_IFA_915MHz_LSM110A`,
`ANT_LSM110A_BreakAwaySlots_EVM_ONLY`, `C_0402_1005Metric_0G` y `TP_Coax_50R_NanoVNA`.

> **Sobre el `.pretty`:** una versión anterior de este README avisaba de que KiCad «espera»
> una carpeta `.pretty` y sugería renombrar si daba problemas. **No hace falta, y está
> comprobado:** el cargador de KiCad enumera esta carpeta plana sin más
> (`IO_MGR.PluginFind(KICAD_SEXP).FootprintEnumerate()` devuelve los cinco). El sufijo
> `.pretty` es una convención que el diálogo de *crear* biblioteca añade solo; para *leer* no
> se exige. **No renombres la carpeta:** la `fp-lib-table` de la placa de prueba y los campos
> `Footprint` de los símbolos apuntan a este nombre.

**Símbolos** — *Manage Symbol Libraries → Project Specific Libraries → `+`*, **dos entradas**
(son dos archivos, y no se pueden fusionar):

| Nickname | Library Path |
|---|---|
| `0G-LockControl` | `${KIPRJMOD}/../Hardware/v0-replica-sji/kicad-lib/LSM110A.kicad_sym` |
| `0G-Antenna` | `${KIPRJMOD}/../Hardware/v0-replica-sji/kicad-lib/0G_Antenna.kicad_sym` |

El campo `Footprint` de los dos símbolos ya viene puesto (`0G-LockControl:LSM110A` y
`0G-LockControl:ANT_IFA_915MHz_LSM110A`), así que **si usas ese nickname para los footprints
el enlace es automático**. Si eliges otro nickname, hay que rehacer los enlaces a mano.

### Nota de versión de KiCad

Los archivos **no están todos en el mismo formato**, y conviene saberlo antes de abrir el
proyecto. Probado con **KiCad 7.0.11**, no supuesto:

| Archivo | Formato | ¿Carga en KiCad 7? |
|---|---|---|
| `0G_Antenna.kicad_sym` | KiCad 7 · `20220914` | ✅ sí |
| Los 4 `.kicad_mod` generados | KiCad 7 · `20221018` | ✅ sí |
| `LSM110A.kicad_mod` | KiCad 8 · `20240108` | ✅ **sí** — el formato de footprint es tolerante |
| `LSM110A.kicad_sym` | KiCad 9 · `20241209` | ❌ **no** — `Unable to load library` |

Los generados salen en formato **KiCad 7 a propósito**: es el que más versiones leen, y KiCad
no convierte hacia atrás.

**Consecuencia práctica:** con **KiCad 9 carga todo**. Con **KiCad 7 u 8** tendrás el módulo
en el PCB pero **no en el esquemático** — su footprint carga y su símbolo no. Si trabajas en
7/8 y necesitas el símbolo, hay que regenerarlo en formato antiguo: es trabajo pendiente, no
un fallo de la biblioteca.

Para saber en qué versión estás: *Help → About KiCad*.

---

## Símbolo — cómo está organizado

**Los 34 pines están completos y verificados.** La distribución es funcional, no física:
se agrupan por uso para que el esquemático se lea, no por número de pad.

**Izquierda (20 pines)** — alimentación, sistema y comunicaciones:

```
VDD (11)
NRST (30) · BOOT0 (31)
PA13/SWDIO (7) · PA14/SWCLK (8)
PA2/UART2_TX (14) · PA3/UART2_RX (13)
PB6/UART1_TX (19) · PB7/UART1_RX (18)
PA9/I2C1_SCL (3) · PA10/I2C1_SDA (4)
PA1/WAKE-UP (15) · PA0 (16)
GND ×7 (1, 10, 12, 20, 23, 32, 34)
```

**Derecha (14 pines)** — RF y GPIO libres:

```
RFOUT (33)
PB2/ADC_IN4 (2) · PA11 (5) · PA12 (6) · PA15 (9) · PB8 (17)
PB3 (21) · PB4 (22) · PA8 (24) · PA7 (25)
PB5/SPI1_MOSI (26) · PA6/SPI1_MISO (27) · PA5/SPI1_SCK (28) · PA4/SPI1_NSS (29)
```

### Tipos eléctricos y qué implica para el ERC

| Pin | Tipo KiCad | Por qué |
|---|---|---|
| `VDD` (11) | `power_in` | El ERC exigirá una `PWR_FLAG` o un símbolo de alimentación. Correcto. |
| `GND` ×7 | `power_in` | **Los 7 hay que conectarlos.** Si dejas uno suelto, el ERC te avisa — es lo que quieres. |
| `PA1` (15) | `input` | La Tabla 5-1-1 lo declara tipo **`I`**, no `I/O`. Es el único pin así. |
| `RFOUT` (33) | `bidirectional` | La tabla lo declara tipo `A` (analógico); KiCad no tiene ese tipo y `bidirectional` es lo correcto para un pin que transmite y recibe. |
| Resto | `bidirectional` | Todos son *General purpose IO* en la Tabla 5-1-1. |

### Las etiquetas alternativas son del **silicio**, no del EVB

`PA9/I2C1_SCL`, `PB5/SPI1_MOSI`, etc. salen de la columna *Description* de la Tabla 5-1-1.

**Deliberadamente NO se usaron** las etiquetas del diseño de referencia de SJI
(`PA8(LED1)`, `PA11(LED2)`, `PA15(LED3)`, `PA12(RF-BUSY)`, `PA0(BUT1)`, `PA1(BUT2)`,
`PB8(RF-IRQ2)`, `PA7(DBG4)`). Esas describen lo que SJI conectó **en su placa**, no
funciones del módulo. Poner `LED1` en un símbolo reutilizable induce a error.

Dato útil de todos modos: la referencia pone sus tres LEDs en **PA8 (24), PA11 (5) y
PA15 (9)** — los tres libres para nosotros, y ninguno choca con UART2.

---

## Footprint — cotas y verificación

| Parámetro | Valor | Fuente |
|---|---|---|
| Cuerpo | **14.00 × 15.00 mm** (2.8 mm alto) | Fig. 5-2-1 |
| Pads | **34**: 12 izq + 10 abajo + 12 der | Fig. 5-3-1 |
| Tamaño de pad (laterales) | **1.20 × 0.60 mm** | Fig. 5-3-1 |
| Tamaño de pad (fila inferior) | **0.60 × 1.20 mm** (girados 90°) | Fig. 5-3-1 |
| Pitch | **1.00 mm** | Fig. 5-3-1 |
| Gap entre pads | **0.40 mm** | Fig. 5-3-1 |
| Vano pin 1→12 y 23→34 | **11.00 mm** | Fig. 5-3-1 |
| Vano pin 13→22 | **9.00 mm** | Fig. 5-3-1 |
| Separación entre columnas | **13.20 mm** centro-centro | derivado (ver nota) |
| **Pad central** | **NINGUNO** | Tabla 5-1-1 no lista ninguno |

Origen en el **centro del cuerpo**. Cada pad sobresale **0.20 mm** del borde del cuerpo
(los pads castellados del módulo miden 1.00 mm, más 0.20 mm de filete exterior).

Capas: `F.Cu/F.Paste/F.Mask` en los pads · marcas de esquina y punto de pin 1 en `F.SilkS` ·
contorno del cuerpo y nota del PSR en `F.Fab` · courtyard en `F.CrtYd`.

### Verificación cruzada contra el footprint de EasyEDA

Se compararon los 34 pads, ambos centrados en su propio centroide:

```
desviación máxima de posición: 0.0160 mm   (un solo pad)
desviación máxima de tamaño:   0.0000 mm
```

La única diferencia está en el **pin 2**, y es **a nuestro favor**: en el footprint de
EasyEDA ese pad está +0.016 mm fuera de retícula (documentado en
`../00-fuente-de-verdad/validacion-footprint.md` §3). **Este footprint lo pone exacto.**
Los otros 33 pads coinciden a cuatro decimales.

### Nota sobre los 13.20 mm

La Fig. 5-3-1 **no acota la separación entre columnas** — da el vano vertical (11.00), el
pitch, el tamaño de pad y el ancho del cuerpo, pero no esa cota. Los 13.20 mm se derivan
de que el pad sobresalga 0.20 mm del borde de un cuerpo de 14.00 mm, y **coinciden
exactamente** con el footprint de EasyEDA (13.1999 mm medidos). Coherente por dos vías,
pero es el único número no citado directamente de una figura.

---

## Dos cosas que este footprint **no** puede imponer, y que hay que hacer en el PCB

1. **PSR coating bajo el módulo** (DS §5.4, pág. 17). El footprint lo rotula en `F.Fab`
   como recordatorio, pero es una nota de fabricación: hay que pedirlo a la casa. Es **H-09**.
2. **Nada de cobre bajo el módulo en la cara superior.** Y en particular: **no** añadir
   un plano con vías térmicas. El `README.md` de la carpeta de EasyEDA dice que existe un
   pad central de GND — **no existe**, y seguir esa instrucción mete cobre exactamente
   donde el DS pide recubrimiento aislante. Es **N-01**.

---

## Reproducir estos archivos

**Los seis de la antena y RF** — un comando, y son byte a byte reproducibles (UUID
deterministas y bloques ordenados, así que regenerar no ensucia el diff de git):

```bash
cd Hardware/KiCad/tools && ./build.sh
```

Regenera los footprints y el símbolo aquí, reconstruye la placa de prueba y corre las **122
comprobaciones** (17 de la cadena de cotas + 63 de footprints + 42 de la placa, con DRC = 0).
Sale con código ≠ 0 si algo falla, así que vale tal cual en CI. Método en
`../../KiCad/docs/verificacion.md`.

**Los dos del LSM110A** se generaron por script en F1, con `assert` sobre los 34 pines, los
tres vanos y el gap; el generador está en el historial de la sesión de F1 y `build.sh` **no**
los toca. Para volver a validar el footprint sin KiCad, el script de comparación contra
EasyEDA está en `../00-fuente-de-verdad/validacion-footprint.md` §«Reproducir estas medidas».

---

## La antena

Cotas, keepout, rendimiento medido de la referencia y el cotejo contra el arte de producción
de SJI: [`../00-fuente-de-verdad/antena-cotas.md`](../00-fuente-de-verdad/antena-cotas.md).
Geometría reconstruida y decisiones de diseño:
[`../../KiCad/docs/geometria-antena.md`](../../KiCad/docs/geometria-antena.md).

Dos cosas del footprint de la antena que sorprenden si no se avisa:

1. **El origen es la esquina superior izquierda del cobre**, no el centro. Para colocarla en
   una placa cuyo borde superior esté en `y_borde`:
   `x = (ancho_placa − 39.50) / 2` (centrada), `y = y_borde + 4.00`.
2. **Los pads 1 y 2 están unidos por cobre a propósito** — es el stub de cortocircuito de la
   IFA. El footprint lo declara como `net_tie`, así que KiCad entiende que el corto es
   intencional y el DRC no lo marca. **No lo «arregles».**

Y una que no es del footprint pero anula la antena si se ignora: hace falta **keepout total
en ambas capas** desde el borde de la placa hasta 20.03 mm — sin cobre, sin plano, sin vías,
sin componentes. En una IFA el plano de tierra es parte del radiador.
