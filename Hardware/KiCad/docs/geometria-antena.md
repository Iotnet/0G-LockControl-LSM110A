# Geometría de la antena — `ANT_IFA_915MHz_LSM110A`

Reconstrucción del plano **«1.5 Antenna Dimension»** (antena PCB integrada, capa TOP)
como geometría paramétrica en `tools/antenna_geometry.py`, de donde se generan el
footprint, el símbolo y la placa de prueba.

**Fuente autoritativa:** `SJIT_LSM110A_UserManual_Rev1.4_240626.pdf`, secciones 1.3
(esquemático), 1.5 (antenna dimension), 1.6 (return loss & VSWR) y 1.9 (EVB radiation →
conduction change). El manual lleva marca «SJIT CONFIDENTIAL», así que **no se versiona en
este repositorio**: se cita por nombre y revisión.

La reconstrucción se verificó **píxel a píxel** contra el bitmap original del manual — ver
[Verificación contra el plano oficial](#verificación-contra-el-plano-oficial).

## Topología

Es una **IFA (Inverted-F Antenna) de placa ranurada**, no un serpentín de pista fina:

- Una placa de cobre de **39.50 × 13.00 mm** en la capa TOP.
- Dos **ranuras de 0.50 mm** que la atraviesan casi de lado a lado, cada una abierta
  por un borde opuesto. Eso parte la placa en tres brazos unidos alternadamente y
  fuerza el recorrido de corriente en forma de «2».
- Un **stub en L de cortocircuito** que une la placa con el plano de tierra: es lo que
  hace de esto una IFA y no un monopolo.
- Una **línea de alimentación de 1.00 mm** pegada al stub, que sale en CPWG 50 Ω hacia la
  red de matching y de ahí al RFOUT del módulo.

El recorrido de la corriente es:

```
feed → brazo inferior → (puente izquierdo, 6.00) → brazo central
     → (puente derecho, 5.50) → brazo superior → extremo abierto
```

Todo el cobre es **un solo polígono conexo y sin huecos**. Eso no es una afirmación de
buena fe: lo comprueba el motor de geometría de KiCad en `tools/verify.py`
(`OutlineCount() == 1`, `HoleCount(0) == 0`).

## Sistema de coordenadas del footprint

| | |
|---|---|
| Origen | esquina **superior izquierda** del cobre de la antena |
| +X | hacia la derecha |
| +Y | hacia **abajo** (convención KiCad) |

Con ese origen, el borde superior de la PCB queda en `y = -4.00` y el borde del plano
de tierra en `y = +16.03`.

## Cotas

### Cadena vertical (cierra exacta con la cota 20.03 del plano)

| Tramo | mm |
|---|---|
| borde superior PCB → borde superior antena | 4.00 |
| brazo superior | 5.00 |
| ranura 1 | 0.50 |
| brazo central | 5.00 |
| ranura 2 | 0.50 |
| brazo inferior | 2.00 |
| **alto del cuerpo de la antena** | **13.00** |
| hueco antena → plano de tierra | 3.03 |
| **borde PCB → plano de tierra** | **20.03** ✔ |

La cota **7.50** del plano corresponde a *brazo central + ranura 2 + brazo inferior*
(5.00 + 0.50 + 2.00), medida desde el borde inferior de la ranura 1.

### Cadena horizontal

| Cota | mm | Nota |
|---|---|---|
| ancho de la PCB de referencia | 50.00 | |
| ancho del cobre de la antena | 39.50 | centrada → 5.25 de margen a cada lado |
| ranura 1 (abierta a la izquierda) | 34.00 | → puente derecho = 39.50 − 34.00 = **5.50** |
| puente izquierdo | 6.00 | → ranura 2 (abierta a la derecha) = **33.50** |
| ancho de ranura | 0.50 | nota 2 del plano |
| línea de feed | 1.00 | centrada en x = 34.00, o sea x ∈ [33.50, 34.50] |
| brazo inferior, borde derecho | 34.50 | alineado con el borde derecho del feed |

### Borde derecho y stub en L

| Cota | mm | Nota |
|---|---|---|
| alto del borde derecho (x = 39.50) | 10.50 | brazo superior + ranura 1 + brazo central ✔ |
| stub en L, tramo horizontal | 5.00 | x ∈ [34.50, 39.50] |
| stub en L, **alto del tramo** | **1.02** | y ∈ [11.98, 13.00] — **derivado de 4.05 − 3.03**, no acotado |
| stub en L, ancho de la pata | 1.00 | x ∈ [38.50, 39.50], cota del plano |
| stub en L, pata vertical | x ∈ [38.50, 39.50] | de y = 13.00 hasta el plano en 16.03 |
| stub en L, bounding box | 5.00 × **4.05** | cota del plano, respetada exacta |

### Línea RF

| Cota | mm |
|---|---|
| ancho de pista (CPWG) | 1.00 |
| gap coplanar (CPWG) | 0.15 |
| borde del plano → pad de C101 | 5.60 |

El feed y el stub son **contiguos** (separación 0.00): esa adyacencia es exactamente la
topología IFA — la alimentación entra justo al lado del punto de cortocircuito, y la
relación entre las dos posiciones es lo que fija la impedancia de entrada.

### Área de cobre

**480.91 mm²** (sin contar el ancla del pad de feed). Se comprueba por dos caminos
independientes que tienen que coincidir: suma de rectángulos y fórmula del zapatero
sobre el contorno cerrado. Esa doble comprobación es la que detectó un vértice mal
puesto en la primera versión del contorno.

## Medir estas cotas en KiCad

**Regla: cada cota vive donde está el cobre que mide.** Una cota que apunta a un sitio donde
no hay nada estorba y hace dudar de la geometría, así que las tres cotas verticales del plano
están repartidas entre el footprint y la placa.

### En el footprint (`Cmts.User`) — las dos que miden cantos propios

| Cota | Va de | a | Dónde está la marca |
|---|---|---|---|
| **3.03** | fondo del cobre, y = 13.00 | borde del plano, y = 16.03 | x = −2.30, a la izquierda |
| **4.05** | techo del tramo del stub, y = 11.98 | borde del plano, y = 16.03 | x = 41.20, a la derecha |

Son marcas testigo en los dos extremos más el tramo entre ellas, dibujadas con líneas: **un
`.kicad_mod` no admite objetos de cota de KiCad**, esos son de placa. Se miden con la regla.

`verify.py` comprueba que las marcas caen exactamente sobre la geometría que dicen medir
(13.00, 11.98, 16.03), y **que no haya ninguna cota por debajo del plano** — donde el
footprint no tiene cobre. Una anotación no puede quedarse desfasada sin que el build falle.

### En la placa de prueba (`User.Drawings`) — la que mide hasta un componente

| Cota | Va de | a |
|---|---|---|
| **5.60** | borde del plano, y = 20.03 | borde del pad 1 de L101, y = 25.63 |

**El 5.60 no cabe en el footprint de la antena.** Va del borde del plano al pad de C101/L101,
y ese condensador es un **componente de placa**: dentro del footprint no hay cobre a esa
altura, así que la cota apuntaría al vacío. Se dibuja en la placa, donde el pad existe, y ahí
sí es un **objeto de cota de KiCad de verdad** — el valor lo calcula KiCad, de modo que si
alguien mueve el componente la cota se actualiza y deja de coincidir con el plano a la vista.
`verify_board.py` comprueba que exista, que mida 5.60, que caiga sobre el eje de la línea RF
y que esté en una capa de documentación y no de cobre.

### Al medir con la regla

> **La regla de KiCad se engancha a la rejilla.** Con rejilla de 0.5 o 1 mm, **3.03 se lee
> como 3.000 y 4.05 como 4.000**, y parece que la geometría está mal cuando lo que está mal
> es la medida. Pon la rejilla en **0.01 mm** (o menor), o mantén **Ctrl** pulsado mientras
> mides para desactivar el enganche.

### Hasta dónde llega el cobre del footprint

El pad de feed baja **1.50 mm por debajo del borde del plano** (hasta y = 17.53) y ahí se
acaba. No es un recorte: es deliberado, para que la pista CPWG pueda **arrancar fuera del
keepout** y aun así caer dentro del pad — una pista de 1.00 mm tiene casquete redondo de
0.50 mm de radio, así que su punto de arranque tiene que quedar a ≥ 0.50 mm del borde.

Los 5.60 mm de línea hasta C101 **no son parte de la antena**: son ruteo. Si el footprint los
llevara, impondría dónde va la red de matching y duplicaría cobre con la pista de la placa.
Por eso el footprint acaba en 17.53 y el resto lo pones tú al rutear.

## Discrepancias encontradas en el plano

Están documentadas porque afectan a decisiones, no por pedantería.

### 1. Stub en L: la cota 4.05 — corregido

> **Una versión anterior implementó 4.03 y estaba mal.** Se dejó documentado que «el rótulo
> 4.05 es el valor redondeado». Ese razonamiento no se sostiene: **redondear 4.03 a dos
> decimales da 4.03, no 4.05.** Un rótulo de CAD se calcula de la geometría; no puede
> aparecer 4.05 sobre una geometría de 4.03.

**De dónde salía el error.** Se supuso que el tramo horizontal del stub medía **1.00** de
alto, igual que el ancho de su pata. Con esa suposición, `STUB_TOP_Y = 13.00 − 1.00 = 12.00`
y el bounding box sale 16.03 − 12.00 = **4.03**. Pero **ese alto no está acotado en el
plano**: era una suposición, no un dato. Y para que la comprobación pasara se escribió con
`tol=0.03`, o sea se ajustó la prueba para que aceptara la suposición. Mal método.

**Cómo se resuelve.** Las dos cotas verticales de esa zona **comparten la flecha inferior**
— medido sobre el bitmap original, las puntas caen en las filas 226.5 / 251.5 (cota 3.03) y
218.5 / 251.5 (cota 4.05); la fila 251.5 es el borde del plano en ambas. Restarlas es
aritmética sobre los valores impresos por SJI, sin depender de píxeles:

```
   4.05   techo del tramo horizontal -> borde del plano
 − 3.03   fondo del cobre de la antena -> borde del plano
 = 1.02   alto del tramo horizontal
```

Ese **1.02 es la única cota de la cadena que el plano deja libre**, y por eso es la que
absorbe la diferencia. Respetando las dos cotas impresas no se contradice nada del dibujo.

**Se implementa 4.05**, con `STUB_V_EXT` como parámetro de entrada y `STUB_RUN_H = 1.02`
derivado. La comprobación es ahora **exacta**, sin tolerancia.

**Por qué la medida no podía decidirlo.** El bitmap del plano tiene 8.203 px/mm
(**0.122 mm/px**) y el arte de producción 15.63 px/mm (**0.064 mm/px**). La diferencia entre
4.03 y 4.05 es **0.02 mm = 0.16 px**. Ninguna de las dos fuentes puede resolverla:

| Fuente | Medido |
|---|---|
| Plano, geometría dibujada (33 px) | 4.02 – 4.05 según con qué cota se calibre |
| Arte de producción FCC pág. 5 | 4.030 ± 0.06 |
| Plano, **texto impreso** | **4.05** |

Las medidas son compatibles con ambos valores; el texto impreso solo con uno. **Manda el
texto impreso**: en una réplica cuyo objetivo es la equivalencia regulatoria, no se
sobrescribe en silencio una cota declarada por el fabricante con un valor derivado de una
suposición propia.

**Impacto eléctrico: ninguno.** 0.02 mm sobre un brazo de stub a 915 MHz es λ/16000, y está
por debajo de la tolerancia de grabado de cualquier fabricante (±0.05 mm o peor). El cambio
es por fidelidad y por honestidad del modelo, no porque mueva la resonancia. El área de
cobre pasa de 480.81 a **480.91 mm²** (+0.10 = 5.00 × 0.02).

### 2. C101: serie o shunt — resuelto por el esquemático oficial

Parecía un conflicto entre dos fuentes:

- La nota 3 del plano dice: *«Línea de alimentación: 1.00 de ancho, con C101 en serie hacia
  la entrada RF»*.
- `Hardware/certificacion-FCC/README.md` dice: *«L101 = 0 Ω serie · C101 = 2.2 pF shunt ·
  C102 = DNI»*.

**El esquemático de la sección 1.3 del User Manual lo cierra.** La nota del plano está
equivocada: `C101` es **shunt**, no serie. La red real es:

```
CON101 (SMA_3P_ST) pin 3 ──┐
                           │
ANT1 pin 1 ────────────────┼── L101 (0R/1005) ──┬── RFOUT
   (PCB-pattern_Antenna)   │                    │
ANT1 pin 2 ── (stub a GND) │                    │
                         C101                 C102
                     2.2pF/1005             DNI/1005
                           │                    │
                          GND                  GND
```

Así que `certificacion-FCC/README.md` era correcto. Tres cosas que confirma este esquemático
y que ya están implementadas tal cual:

| Posición | Designador | Referencia | Dónde va |
|---|---|---|---|
| serie | `L101` | 0 Ω / 1005 | entre el nodo de antena y RFOUT |
| shunt | `C101` | 2.2 pF / 1005 | lado **antena** |
| shunt | `C102` | DNI / 1005 | lado **radio** |

La figura de matching de la sección 1.5 dibuja lo mismo simplificado: colapsa el 0 Ω a un
cable y omite el DNI, dejando solo `C101` resaltado en rojo por ser el elemento de ajuste.

**El símbolo de antena del esquemático oficial tiene DOS pines**, igual que el símbolo de
este proyecto. El pin 2 es el stub de cortocircuito. Eso valida por separado el enfoque de
net-tie del footprint.

**`CON101` mide el módulo, no la antena.** Su pin 3 cuelga del nodo *de antena*, así que
sirve para medida conducida **después** de arrancar la antena y quitar `C101` (sección 1.9).
En la placa de prueba de este proyecto el puerto de VNA (`J1`) va en el lado **RFOUT**, que
es el punto correcto para medir la antena: `J1 → C102 → L101 → C101 → antena`, o sea la red
de referencia con el VNA en el sitio del módulo.

**Pregunta cerrada — el DC:** con `L101 = 0 Ω` y el stub de la IFA a masa, RFOUT queda
cortocircuitado a masa en continua. No es un descuido: es lo que hace el diseño certificado
de SJI, y el S11 medido de la sección 1.6 se tomó justo con esa población. No hace falta
condensador de bloqueo.

### 3. Fila de ranuras: no es RF, es una línea de troquelado

Las cinco ranuras redondeadas que el plano dibuja en magenta entre la antena y el plano de
tierra parecían un detalle de RF sin cotar. **No lo son.** La sección 1.9 del User Manual
(«EVB Radiation → Conduction Change») las explica:

1. **PCB ANT remove** — se arranca la sección de antena por esa línea
2. **C101 (Capacitor) remove**
3. **CON101 RF SMA Connector insertion**
4. Change complete

Y el esquemático de la sección 1.3 marca esa misma zona con un recuadro de trazos rotulado
**«CUT»** que encierra `ANT1` y `C101`. O sea: son la línea por la que se **rompe** la placa
para pasar de medida radiada a medida conducida en el EVM.

Eso explica por qué dejan tan poco material: el borde inferior de las ranuras queda a unos
**0.2 mm** del borde del plano. No es un error de fabricación, es debilitamiento
intencional para que rompa por ahí.

**Consecuencia de diseño: en el producto NO deben ir.** Una antena pensada para arrancarse
a mano es un punto de fallo mecánico en una cerradura. Se entregan en un footprint aparte,
`ANT_LSM110A_BreakAwaySlots_EVM_ONLY`, cuyo nombre ya lo dice.

**Cotas medidas** sobre el bitmap original (8.203 px/mm → ±0.12 mm). El plano no las cota,
así que esto es lo mejor que se puede sacar sin el Gerber:

| Ranura | x inicio | x fin | ancho | qué la limita |
|---|---|---|---|---|
| A | −2.50 | 7.31 | 9.81 | 2.7 mm al borde izquierdo de la PCB |
| B | 10.06 | 20.36 | 10.30 | |
| C | 23.16 | 33.16 | 10.00 | acaba 0.34 mm antes de la línea de feed |
| D | 34.93 | 38.22 | 3.29 | entre el feed (34.50) y la pata del stub (38.50) |
| E | 39.93 | 42.49 | 2.56 | arranca 0.43 mm tras la pata del stub |

Vertical: **y 14.08 … 15.85** (alto **1.77 mm**), o sea 1.08 mm por debajo del cobre de la
antena y ~0.2 mm por encima del borde del plano.

## Verificación contra el plano oficial

El plano de la sección 1.5 va incrustado en el PDF como **bitmap de 522 × 417 px**, no como
vector. Escalando por el ancho conocido de la antena (39.50 mm = 324 px) sale **8.203 px/mm**,
o sea 0.12 mm por píxel. A esa resolución se puede localizar cada borde de cobre y comparar.

El original dibuja el cobre como **contorno** (verde), no relleno, así que cada arista del
cobre es una línea localizable. Resultado del barrido:

### Niveles verticales — los seis caen exactos

| Rasgo | y del plano | píxel medido | y reconstruido |
|---|---|---|---|
| borde superior de la antena | 0.00 | y=119.5 (origen) | **0.00** |
| ranura 1, borde superior | 5.00 | y=160–162 → 4.94–5.18 | **5.00** |
| ranura 1, borde inferior | 5.50 | y=164–166 → 5.42–5.67 | **5.50** |
| ranura 2, borde superior | 10.50 | y=206–207 → 10.54–10.67 | **10.50** |
| ranura 2, borde inferior | 11.00 | y=209–211 → 10.91–11.15 | **11.00** |
| borde inferior de la antena | 13.00 | y=226–227 → 12.98–13.10 | **13.00** |
| borde del plano de tierra | 16.03 | y=251–252 → 16.03–16.15 | **16.03** |

### Aperturas de las ranuras — confirmadas por el mapa de píxeles

Esto es lo que más importaba, porque define la topología en «2»:

- **Ranura 1, borde izquierdo (x = 0):** hay cobre a y = 5.06 y a y = 5.55, pero **hueco a
  y = 5.30**. La ranura llega al borde → **abierta a la izquierda** ✔
- **Ranura 2, borde derecho (x = 39.50):** cobre hasta y = 10.67, **nada entre 10.79 y
  11.15**, y otra vez cobre desde 12.01. La ranura llega al borde → **abierta a la
  derecha** ✔, y además el hueco sigue hasta 12.00, lo que confirma que el brazo inferior
  **no** llega al borde derecho: lo que hay a partir de 12.00 es el stub.

### Extremos horizontales

| Rasgo | medido | reconstruido |
|---|---|---|
| ancho del cobre | x 107…431 px = 39.50 | **39.50** |
| ranura 1: del borde izq. a | x 387 px → 34.15 | **34.00** |
| ranura 2: arranca en | x 155 px → 5.85 | **6.00** (puente izquierdo) |
| brazo inferior, borde derecho | x 390 px → 34.50 | **34.50** |
| stub, borde superior | y 218 px → 12.01, x 391…432 | **12.00**, x 34.50…39.50 |
| stub, pata vertical (borde izq.) | x 423 px → 38.52 | **38.50** |
| línea de feed (bordes) | x 382 → 33.53 y x 390 → 34.50 | **33.50 / 34.50** |
| **gap coplanar del CPWG** | x 380–381 → 33.28–33.40, junto a 33.53 | **0.15** |
| plano de tierra (bordes) | x 69 → −4.63 y x 472 → 44.50 | ancho ≈ **50.00**, antena centrada |

El gap del CPWG aparece dibujado en el propio plano de la antena: hay una línea de plano a
0.13 mm del borde de la línea de feed, a los dos lados. Coincide con el 1.00 / 0.15 que da
el User Manual para la línea RF.

### El stub sí toca el plano

En el nivel del borde del plano (y = 16.03) la línea de contorno se **interrumpe** entre
x = 38.65 y x = 39.50 — justo el ancho de la pata del stub. O sea que el cobre del stub y el
del plano son la misma pieza: **el cortocircuito a masa de la IFA está en el plano original**.
Y se interrumpe otra vez entre x = 33.65 y 34.50, donde cruza la línea de feed, que ahí sí
va aislada por el gap del CPWG.

Conclusión: **la reconstrucción coincide con el plano oficial en todos los rasgos medibles.**
Lo que el bitmap no puede resolver es el detalle por debajo de ~0.12 mm (radios de esquina,
compensaciones de grabado). Para eso seguiría haciendo falta el Gerber.

## Comportamiento RF medido (sección 1.6 del User Manual)

Ya no hay que especular con la frecuencia de resonancia: SJI publica el S11 y el VSWR
medidos de esta antena, con la población de referencia (`L101` = 0 Ω, `C101` = 2.2 pF,
`C102` = DNI) sobre la PCB de 50 × 80 mm. Barrido de 700 MHz a 1.1 GHz.

| Frecuencia | S11 | VSWR | |
|---|---|---|---|
| 863.0 MHz | −8.00 dB | 2.33 | EU868 |
| 865.0 MHz | −8.27 dB | 2.26 | EU868 |
| 868.1 MHz | −8.69 dB | 2.17 | EU868 |
| **902.1 MHz** | **−16.43 dB** | **1.37** | RC2/RC4, borde inferior |
| **921.6 MHz** | **−17.6 dB** | ~1.3 | RC2/RC4 |
| **928.0 MHz** | **−29.28 dB** | ~1.07 | RC2/RC4, borde superior |

Lo que se lee de ahí:

- **En 902 – 928 MHz el S11 está por debajo de −16 dB en toda la banda.** Muy holgado
  frente al −10 dB habitual como objetivo. La antena está bien resuelta para RC2/RC4.
- **La resonancia cae justo por encima de 928 MHz** (el fondo del pozo del barrido está
  fuera de la banda, hacia ~935–945 MHz). Es decir: la banda de trabajo se apoya en el
  flanco de la resonancia, no en su fondo. Eso es normal y le da ancho de banda.
- **Para EU868 esta antena NO sirve tal cual:** −8.7 dB a 868 MHz es VSWR 2.17, o sea ~13 %
  de la potencia reflejada. Habría que alargar el recorrido de corriente (reducir los
  puentes de 5.50 / 6.00) y volver a ajustar. Por eso el footprint ya no lleva la etiqueta
  «868MHz» que tenía al principio.

### Geometría en longitudes de onda

A 915 MHz, λ₀ = 327.6 mm.

| | |
|---|---|
| Huella de la antena | 39.50 × 13.00 mm = 0.121 λ₀ × 0.040 λ₀ |
| Plano de tierra (referencia 50 × 80) | 50 × 60 mm = 0.153 λ₀ × 0.183 λ₀ |
| Recorrido de corriente (línea media) | ≈ 112 mm |

El recorrido por línea media (112 mm) **no** predice la resonancia: los brazos miden 5 mm
de ancho (0.025 λ₀) y las ranuras solo 0.50 mm, así que la corriente no sigue la línea media
y el acoplamiento capacitivo a través de las ranuras es fuerte. Sirve para comparar
variantes entre sí, nada más. Para predecir hace falta un solver EM; para saberlo, la tabla
de arriba.

## Requisitos de integración que hereda el footprint

Estos no son recomendaciones: si no se cumplen, la antena no resuena donde se midió.

1. **Borde superior de la PCB a 4.00 mm** por encima del origen del footprint.
2. **Plano de tierra arrancando exactamente en y = 16.03**, en ambas capas. Ni antes
   ni después: en una IFA el borde del plano es parte de la antena.
3. **Nada de cobre, plano, vías ni componentes** en toda la región por encima de ese
   borde, en las dos capas. El footprint lo impone con un *rule area* propio y un
   courtyard que cubre la misma zona.
4. **Costura de vías** justo por debajo del borde del plano, para que las dos capas
   estén al mismo potencial RF ahí.
5. **Línea RF en CPWG 1.00 / 0.15** desde el pad de feed.
6. El plano de tierra del producto **debe parecerse al de la placa de prueba**. Si cambia
   de tamaño, la antena se desintoniza y hay que volver a ajustar.
