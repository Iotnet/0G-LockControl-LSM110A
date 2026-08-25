# Geometría de la antena — `ANT_IFA_915MHz_LSM110A`

Reconstrucción del plano **«1.5 Antenna Dimension»** (antena PCB integrada, capa TOP)
como geometría paramétrica en `tools/antenna_geometry.py`, de donde se generan el
footprint, el símbolo y la placa de prueba.

## Topología

Es una **IFA (Inverted-F Antenna) de placa ranurada**, no un serpentín de pista fina:

- Una placa de cobre de **39.50 × 13.00 mm** en la capa TOP.
- Dos **ranuras de 0.50 mm** que la atraviesan casi de lado a lado, cada una abierta
  por un borde opuesto. Eso parte la placa en tres brazos unidos alternadamente y
  fuerza el recorrido de corriente en forma de «2».
- Un **stub en L de cortocircuito** que une la placa con el plano de tierra: es lo que
  hace de esto una IFA y no un monopolo.
- Una **línea de alimentación de 1.00 mm** pegada al stub, con **C101 en serie** hacia
  la entrada RF.

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
| stub en L, ancho | 1.00 | y ∈ [12.00, 13.00] en el tramo horizontal |
| stub en L, pata vertical | x ∈ [38.50, 39.50] | de y = 13.00 hasta el plano en 16.03 |
| stub en L, bounding box | 5.00 × **4.03** | el plano rotula 4.05 — ver discrepancias |

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

**480.81 mm²** (sin contar el ancla del pad de feed). Se comprueba por dos caminos
independientes que tienen que coincidir: suma de rectángulos y fórmula del zapatero
sobre el contorno cerrado. Esa doble comprobación es la que detectó un vértice mal
puesto en la primera versión del contorno.

## Discrepancias encontradas en el plano

Están documentadas porque afectan a decisiones, no por pedantería.

### 1. Stub en L: 4.05 rotulado vs 4.03 derivado

La cadena de cotas principal cierra exacta: 4.00 + 13.00 + 3.03 = 20.03. Con esa cadena,
la altura del bounding box del stub sale **4.03**, no el 4.05 que rotula la nota 4.
Diferencia de **0.02 mm**, muy por debajo de la tolerancia de fabricación (±0.1 mm típico
en JLCPCB) y sin efecto medible a 915 MHz.

**Se implementó 4.03**, para no romper la cadena 20.03 que sí está cotada dos veces.

### 2. C101: serie (plano) vs shunt (expediente FCC del repo)

- El plano dice: *«Línea de alimentación: 1.00 de ancho, con C101 en serie hacia la
  entrada RF»* (nota 3).
- `Hardware/certificacion-FCC/README.md` dice: *«Matching network (pin 33 RFOUT → antena):
  L101 = 0 Ω serie · C101 = 2.2 pF shunt»*.

**Se implementó en serie**, siguiendo el plano. Además de ser lo que dice el plano, en una
IFA el condensador serie hace de bloqueo de DC: el cobre de la antena está unido a masa
por el stub, así que sin él la salida del PA queda cortocircuitada a masa en continua.

**Pendiente:** confirmar contra el esquemático de referencia de SJI qué designador va en
cada posición. Puede que ambas fuentes sean correctas y se refieran a componentes
distintos de la misma red.

### 3. Fila de ranuras mecánicas: sin cotar

El plano dibuja en magenta cinco ranuras redondeadas en el hueco entre la antena y el
plano (nota 7: *«Magenta = contorno y ranuras de la PCB»*), pero **no las cota**.

Se entregan aparte, en `ANT_LSM110A_SlotRow_OPTIONAL.kicad_mod`, y **no** forman parte
del footprint principal. Sus coordenadas salen de medir el trazado, con el arranque de
las ranuras D y E corrido a la derecha para dejar ≥ 0.35 mm al cobre del feed y del stub.

No se metieron en el footprint principal por criterio: **no se pone un corte mecánico
cotado a ojo en un contorno de fabricación.** Verificar contra el plano original de SJI y,
si se confirman, colocar el footprint opcional con el mismo origen que la antena.

## Números RF de primer orden

A 915 MHz, λ₀ = 327.6 mm.

| | |
|---|---|
| Huella de la antena | 39.50 × 13.00 mm = 0.121 λ₀ × 0.040 λ₀ |
| Plano de tierra (referencia 50×80) | 50 × 60 mm = 0.153 λ₀ × 0.183 λ₀ |
| Recorrido de corriente (línea media) | ≈ 112 mm |
| ε_eff estimada (sin plano debajo) | ≈ (1 + 4.3)/2 = 2.65 → λ_eff ≈ 201 mm, λ_eff/4 ≈ 50 mm |

**Lo que estos números NO dicen:** la frecuencia de resonancia. Los brazos miden 5 mm de
ancho (0.025 λ₀) y las ranuras solo 0.50 mm, así que la corriente no sigue la línea media
y el acoplamiento capacitivo a través de las ranuras es fuerte. La estimación de 112 mm
por línea media sobreestima mucho la longitud eléctrica; la resonancia real la fijan las
longitudes de ranura y ese acoplamiento.

Sirven para comparar variantes del diseño entre sí. **No sustituyen a medir**, ni a una
simulación EM. El procedimiento de ajuste está en `verificacion.md`.

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
