# Librería KiCad del LSM110A

Generada en **F1** a partir de los datos verificados del datasheet. No es una conversión del
JSON de EasyEDA: los números salen directos de las figuras del DS, y el footprint se
**cotejó después** contra el de EasyEDA como comprobación cruzada.

| Archivo | Qué es | Fuente |
|---|---|---|
| `LSM110A.kicad_sym` | Símbolo de esquemático, 34 pines | DS R08 **Tabla 5-1-1**, págs. 14–15 |
| `LSM110A.kicad_mod` | Footprint LGA-34 | DS R08 **Fig. 5-3-1** y **Fig. 5-2-1**, pág. 16 |

---

## Instalar en KiCad

**Símbolo** — *Preferences → Manage Symbol Libraries → Project Specific Libraries → `+`*

| Campo | Valor |
|---|---|
| Nickname | `0G-LockControl` |
| Library Path | `${KIPRJMOD}/../Hardware/v0-replica-sji/kicad-lib/LSM110A.kicad_sym` |
| Format | KiCad |

**Footprint** — *Preferences → Manage Footprint Libraries → Project Specific → `+`*

| Campo | Valor |
|---|---|
| Nickname | `0G-LockControl` |
| Library Path | `${KIPRJMOD}/../Hardware/v0-replica-sji/kicad-lib` |
| Format | KiCad |

> KiCad espera que las librerías de footprint sean una **carpeta `.pretty`**. Si te da
> problemas, renombra la carpeta a `0G-LockControl.pretty` y apunta ahí. El `.kicad_mod`
> también se puede importar suelto desde el editor de footprints
> (*File → Import → Footprint*).

El campo `Footprint` del símbolo ya viene puesto a `0G-LockControl:LSM110A`, así que si usas
ese nickname el enlace es automático.

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

Ambos se generaron por script, con `assert` sobre los 34 pines, los tres vanos y el gap.
El generador está en el historial de la sesión de F1. Para volver a validar el footprint
sin KiCad, el script de comparación contra EasyEDA está en
`../00-fuente-de-verdad/validacion-footprint.md` §«Reproducir estas medidas».
