# Antena de referencia — cotas, keepout y rendimiento medido

**Fase:** F1 · **Fecha:** 2026-08-20
**Revisión R4:** 2026-08-27 — §2.2, §2.3 y §2.4 corregidas y §2.5 añadida tras medir el **arte de producción**. La interpretación original de la cota `0.50` era errónea; ver §2.3.
**Fuentes:** UM Rev1.4 **§1.5 (pág. 8)** — cotas · **§1.6 (pág. 9)** — return loss y VSWR medidos · **§1.9 (pág. 12)** — pestaña · FCC **pág. 5** — **arte de producción** de las capas top/bottom (no un render ilustrativo: es el cobre real, y es la fuente que resolvió §2.3 y §2.4)
**Método (R3):** pág. 8 renderizada a **600 dpi** y recortada con PIL; zona del feed reescalada ×1.5 para leer el cúmulo de cotas pequeñas. `pdftotext` de la pág. 8 devuelve solo `[ Antenna Pattern ]` y `[ Matching ]` — **toda la ingeniería está en el dibujo.**
**Método (R4):** `certificacion-FCC/user-manual-antenna-trace-design.pdf` **pág. 5** renderizada a **300 dpi**. Esa página no es un plano acotado: es el **arte real de las capas** (top en verde sobre plano azul, bottom en rosa). Se segmenta por color y se mide en píxeles. Escala derivada del contorno de placa, que el propio exhibit acota `50 mm × 80 mm`: **782 px / 50.00 mm = 15.640 px/mm** en X y **1250 px / 80.00 mm = 15.625 px/mm** en Y — concuerdan al **0.1 %**, así que la escala es isótropa y vale **15.6325 px/mm** (resolución **0.064 mm/px**). Detalle en §2.5.

---

## 1. Advertencia de alcance

**F1 registra las cotas tal como el dibujo las da. No reconstruye la geometría.**

El dibujo de la pág. 8 es un plano acotado, no una lista de coordenadas: da longitudes y anchos, pero **no da el origen ni todas las cotas necesarias para cerrar el polígono de forma única**. Reconstruir la IFA como lista de segmentos con coordenadas es trabajo de **F5**, y F5 va a tener que resolver ambigüedades — están señaladas en §3.

Cualquier cota de este documento que F5 use para generar cobre debe re-verificarse contra el render a 600 dpi. La lista de abajo es fiel a lo legible; la interpretación geométrica no está verificada.

> **R4 — el alcance de F1 se quedó corto, y por eso hubo un error.** Registrar cotas «tal como el dibujo las da» no protege de **asignar cada cota al elemento equivocado**, que es exactamente lo que pasó con el `0.50` (§2.3). Un plano acotado se puede leer bien y aun así interpretarse mal.
>
> La lección de método: **la pág. 5 del exhibit de FCC no es una fuente secundaria de los renders, es el arte de las capas**, y por tanto es *más* autoritativa que el plano acotado — no da cotas, pero da el cobre. Cotejar el plano contra el arte no era opcional. Toda cota de este documento marcada «confirmada en §2.5» ya pasó ese cotejo.
>
> La reconstrucción geométrica que F1 declaraba fuera de alcance **ya existe y está verificada**: `Hardware/KiCad/tools/antenna_geometry.py` (fuente de verdad paramétrica) y `Hardware/KiCad/docs/geometria-antena.md`. Este documento sigue siendo el registro de lo que dicen las fuentes de SJI; el cobre lo define el módulo de Python.

---

## 2. Cotas legibles (UM §1.5, pág. 8) — todas en mm

### 2.1 Pestaña (contorno de placa, trazo magenta)

| Cota | Valor | Confianza |
|---|---|---|
| Ancho de la pestaña | **50.00** | alta — cota principal, dígitos grandes |
| Alto de la pestaña | **20.03** | alta |

El `50.00` coincide con el ancho total de la placa de referencia (**50 × 80 mm**, FCC pág. 5). La pestaña ocupa el **borde superior completo** de la placa.

### 2.2 Brazos radiantes (cobre, trazo verde)

| Cota | Valor | Qué acota | Confianza |
|---|---|---|---|
| Brazo 1 (superior) | **39.50** | longitud horizontal del trazo más alto — es también el **ancho total del cobre** | alta · **confirmada en §2.5** (39.54) |
| Brazo 2 | **34.00** | longitud horizontal del segundo trazo = largo de la **ranura 1** | alta · **confirmada en §2.5** (34.03) |
| **Alto** del brazo 1 (superior) | **5.00** | vertical, medida en el extremo izquierdo. **Es el ALTO del brazo, no la separación entre brazos** — ver §2.3 | alta · **confirmada en §2.5** (4.99) |
| **Alto** del brazo 2 (central) | **5.00** | vertical, medida en el centro. Ídem | alta · **confirmada en §2.5** (4.99) |
| Caída izquierda | **7.50** | vertical, del brazo 2 hacia abajo = `0.50 + 5.00 + 2.00`, o sea del techo del brazo central al borde inferior del cobre | alta · **cierra aritméticamente** |
| Caída derecha | **10.50** | vertical, del brazo 1 al techo de la ranura 2 = `5.00 + 0.50 + 5.00`. Es el alto del cobre en el **borde derecho** (la ranura 2 se abre por ahí) | alta · **cierra aritméticamente** |
| Retorno derecho | **5.00** | horizontal, del eje del feed (x 34.00) al eje de la pata del stub (x 39.00) | alta · **confirmada en §2.5** (39.02 − 34.03 = 4.99) |
| Alto del brazo 3 (inferior) | **2.00** | vertical, cerca del extremo izquierdo del brazo 3 | ~~baja~~ → alta · **confirmada en §2.5** (1.92) |
| Hueco antena ↔ plano | **3.03** | vertical, zona inferior central | ~~baja~~ → alta · **confirmada en §2.5** (3.13) |
| — | **4.05** | vertical, lado derecho, zona de las ranuras = bounding box de la pata del stub | media (la reconstrucción cierra en **4.03**; ver §2.5) |
| — | **5.60** | vertical, del borde inferior de la pestaña al pad de C101 | alta |

**Cierre dimensional (era la deuda §3.4, ahora resuelta):**

```
  4.00   borde superior de la placa -> borde superior del cobre
 13.00   alto del cuerpo de la antena  =  5.00 + 0.50 + 5.00 + 0.50 + 2.00
  3.03   borde inferior del cobre -> borde del plano de tierra
 ------
 20.03   =  alto de la pestaña   (cota principal de §2.1)
```

Suma exacta, sin holguras ni conversiones centro-a-borde. **Las verticales sí cerraban** — lo que faltaba era contar los `0.50` como ranuras y no como pistas.

### 2.3 Anchos de pista — ⚠ CORREGIDO EN R4

> **La lectura original de esta sección era errónea.** Decía:
> *«Pista radiante 0.50 — brazos meandreados… La pista radiante es de 0.50 mm y el feed de 1.00 mm. Son distintos: no uniformar.»*
>
> **No hay pista radiante de 0.50 mm, y la antena no es un meandro.** El `0.50` es el **ancho de las RANURAS**, no del cobre.

La antena es una **placa ranurada**, no un serpentín: un rectángulo macizo de **39.50 × 13.00 mm** al que se le abren **dos ranuras de 0.50 mm**, cada una por un borde opuesto. Eso obliga a la corriente a recorrer una «2» sin que haya en ningún momento una pista fina. Los `0.50` del plano acotan esos dos huecos — y por eso aparecen exactamente dos veces.

| Elemento | Ancho | Dónde | Estado |
|---|---|---|---|
| Brazo superior (cobre) | **5.00** | de `y=0.00` a `y=5.00` | confirmado §2.5 (4.99) |
| **Ranura 1** (hueco) | **0.50** | de `y=5.00` a `y=5.50`, **abierta por el borde IZQUIERDO**, largo 34.00 | confirmado §2.5 (0.51 / 34.03) |
| Brazo central (cobre) | **5.00** | de `y=5.50` a `y=10.50` | confirmado §2.5 (4.99) |
| **Ranura 2** (hueco) | **0.50** | de `y=10.50` a `y=11.00`, **abierta por el borde DERECHO**, largo 33.50 | confirmado §2.5 (0.51 / 33.52) |
| Brazo inferior (cobre) | **2.00** | de `y=11.00` a `y=13.00` | confirmado §2.5 (1.92) |
| Feed | **1.00** | vertical centrado en `x = 34.00`, baja al matching | confirmado §2.5 (1.02 @ 34.03) |
| Stub de cortocircuito | **1.00** | vertical en `x = 38.50…39.50`, baja al plano de tierra | confirmado §2.5 (1.02 @ 39.02) |

**Lo que sí era correcto:** el feed y el stub miden **1.00 mm** y son distintos del resto. **Lo que hay que descartar:** que exista pista radiante de 0.50 mm.

**Por qué importa, y no es un detalle de redacción.** Un meandro de 0.50 mm y una placa ranurada de 39.50 × 13.00 mm **no son la misma antena**: no tienen el mismo área de cobre (la real son **480.81 mm²**), ni la misma distribución de corriente, ni el mismo ancho de banda. Precisamente el ancho de banda es lo que explica §5 — que la referencia resuene ~925 MHz y aun así dé −16.4 dB en 902.1 es comportamiento de **placa ranurada**; un meandro fino de esa longitud eléctrica sería mucho más estrecho y no cubriría 902–928. Haber redibujado un meandro habría dado una antena que ni cumple el requisito de FCC (§4 de `user-manual-antenna-trace-design`: *misma traza*) ni cubre la banda.

### 2.4 Zona del feed — ⚠ CORREGIDO EN R4

> La lectura original hablaba de **tres** verticales de 1.00 mm: uno al pad de C101, uno con la flecha `Signal Input`, y un tercero por el borde derecho. **En el arte real hay DOS**, no tres (§2.5, barridos horizontales).

Entre el borde inferior del cobre (`y = 13.00`) y el borde del plano de tierra (`y = 16.03`) cruzan **exactamente dos verticales de 1.00 mm**:

| Vertical | Eje | Papel | Cómo se distingue en el arte |
|---|---|---|---|
| **Feed** | `x = 34.00` | señal: baja al matching (L101 → C101 → CON101, rotulados en la pág. 5) y de ahí al RFOUT del módulo | va **rodeado de un canal libre de plano** — el clearance coplanar del CPWG |
| **Stub de cortocircuito** | `x = 39.00` (cobre `38.50…39.50`) | masa: **toca el plano de tierra**, es la pata «F» de la IFA | **se fusiona con el plano, sin clearance** |

El feed nace justo donde termina la ranura 1 (`x = 34.00`), y el borde derecho del stub coincide con el borde derecho del cobre (`x = 39.50`). Están separados **5.00 mm de eje a eje**, que es la cota «retorno derecho» de §2.2.

**El «tercer vertical» era, con toda probabilidad, un artefacto de lectura:** en el plano acotado el feed se dibuja como **dos líneas paralelas** con la cota `1.00` entre ellas, y a 600 dpi ese par se puede leer como dos conductores. Es coherente con que F1 anotara que «el `1.00` aparece tres veces»: son el ancho del feed, el ancho del stub y el tramo horizontal del stub.

`C101` es el 2.2 pF de `red-rf.md` §1 (shunt a GND del lado de la antena) y **está físicamente al pie de la antena, no junto al módulo**. Dato de layout para F5/F6: el condensador de sintonía va abajo, en la placa, inmediatamente bajo la línea de corte de la pestaña. En la pág. 5 se leen los tres rótulos en serie por la línea de feed — `CON101`, `C101`, `L101` — lo que confirma la topología de `red-rf.md`.

### 2.5 Medidas sobre el arte de producción (FCC pág. 5, 300 dpi) — R4

Escala **15.6325 px/mm**, resolución **0.064 mm/px**, derivada del contorno de placa que el propio exhibit acota `50 × 80 mm` (X e Y concuerdan al 0.1 %). El error de una medida es de ±1 px ≈ **±0.06 mm**, más el umbral de segmentación de color.

**Barrido vertical, repetido en `x = 10.00`, `20.00` y `30.00 mm` del borde izquierdo del cobre.** Las tres columnas dan el **mismo** resultado al píxel, así que no es una lectura afortunada:

| Tramo | Medido | Nominal | Δ |
|---|---|---|---|
| Brazo superior | **4.99** | 5.00 | −0.01 |
| **Ranura 1** | **0.51** | 0.50 | +0.01 |
| Brazo central | **4.99** | 5.00 | −0.01 |
| **Ranura 2** | **0.51** | 0.50 | +0.01 |
| Brazo inferior | **1.92** | 2.00 | −0.08 |

Ese patrón `cobre grueso / hueco fino` repetido es la prueba directa de que el `0.50` acota **huecos**. Si la antena fuese un meandro de 0.50 mm, el barrido daría lo contrario: tramos de cobre de 0.51 separados por huecos de ~5 mm.

**Barrido horizontal por el eje de cada ranura** — resuelve por qué borde se abre cada una:

| Ranura | Hueco medido | Puente de cobre | Nominal |
|---|---|---|---|
| 1 (`y ≈ 5.25`) | `x 0.00 … 34.03` → **abierta a la IZQUIERDA** | derecho, **5.50** | largo 34.00 · puente 5.50 |
| 2 (`y ≈ 10.75`) | `x 6.01 … 39.53` → **abierta a la DERECHA** | izquierdo, **5.95** | largo 33.50 · puente 6.00 |

**Barrido horizontal entre el cobre y el plano** (`y = 13.7`, `14.5`, `15.3` mm), tres alturas, mismo resultado: **dos** conductores de **1.02 mm**, con ejes en **34.03** y **39.02 mm**.

**Cotas globales:**

| Cota | Medido | Nominal | Δ |
|---|---|---|---|
| Ancho del cobre | **39.54** | 39.50 | +0.04 |
| Borde de placa → borde del cobre | **4.03** | 4.00 | +0.03 |
| Borde de placa → borde del plano | **20.09** | 20.03 | +0.06 |
| Cobre → plano de tierra | **3.13** | 3.03 | +0.10 |

Todas las desviaciones están por debajo de **0.10 mm** ≈ 1.6 px, o sea dentro del error del método. **El plano acotado (UM §1.5) y el arte de producción (FCC pág. 5) describen la misma geometría, y la reconstrucción de `antenna_geometry.py` coincide con ambos.**

Los scripts de medida no se versionan (son de un solo uso, sobre un PDF que sí está en el repo); el procedimiento de arriba basta para repetirlos.

---

## 3. Lo que el dibujo no cerraba — estado en R4

Las cinco deudas se plantearon para F5. **Cuatro están cerradas**; queda una.

1. ✅ **RESUELTA — cuál vertical es el feed y cuál el cortocircuito.** No son tres, son dos (§2.4). El arte los distingue sin ambigüedad por el **clearance**: el de `x = 34.00` va rodeado de un canal libre de plano (es señal, y baja al matching), el de `x = 39.00` **se fusiona con el plano** (es el cortocircuito). Era «la ambigüedad más importante» y la resolvió el arte, no el plano.
2. ✅ **RESUELTA — origen de coordenadas.** `antenna_geometry.py` fija el datum en la **esquina superior izquierda del cobre de la antena**, con **+Y hacia abajo** (convención KiCad), y lo declara en su docstring. No es la propuesta de F1 (esquina inferior-izquierda de la pestaña), pero está declarado y todo se deriva de él.
3. ✅ **RESUELTA — las cotas `2.00` y `3.03`.** Confirmadas sobre el arte: **1.92** y **3.13** medidas (§2.5), o sea las nominales dentro del error. Suben de confianza baja a alta.
4. ✅ **RESUELTA — cierre dimensional.** Cierra exacto: `4.00 + 13.00 + 3.03 = 20.03` (§2.2). Las verticales legibles no sumaban porque los `0.50` se estaban contando como pistas y no como ranuras — el propio criterio que F1 escribió («si las verticales no cierran contra los 20.03, la geometría está mal interpretada») era correcto y **detectó el error de §2.3**. `antenna_geometry.py` deja esa suma como comprobación automática, así que no puede volver a romperse en silencio.
5. ⚠ **ABIERTA — geometría de las ranuras de desprendimiento.** Siguen sin estar acotadas en ninguna fuente. Ver §6.
   **Novedad de R4:** están **medidas** (no acotadas) sobre el bitmap del UM en `antenna_geometry.py` → `SLOTROW_Y0/Y1`, `SLOTROW_X`, con incertidumbre ±0.12 mm, y existe un footprint aparte, `ANT_LSM110A_BreakAwaySlots_EVM_ONLY`, deliberadamente separado del de la antena.
   **Y la decisión que F1 pedía plantear ya está tomada: en el producto NO van.** Razonamiento en `Hardware/KiCad/docs/geometria-antena.md`; resumen en §6.

---

## 4. Keepout y plano de tierra (FCC pág. 5, renders top/bottom)

Los renders del exhibit muestran esto sin ambigüedad, y es tan importante como las cotas:

- **Top:** el patrón de la antena (verde) vive en una zona **sin plano de tierra**. El plano (azul) **arranca exactamente en el límite inferior de la pestaña** y cubre el resto de la placa.
- **Bottom:** en la zona de la antena **tampoco hay cobre**. El plano de la cara inferior arranca en la misma línea que el de la superior.

**Regla para F5/F6:** dentro del área de la pestaña, **cero cobre en ambas capas** salvo el propio patrón de la antena y sus tres verticales. El plano de tierra empieza en el borde de la pestaña, en las dos capas, alineado.

> **Por qué no se negocia (trampa 8 de la guía):** una IFA no es el patrón — es el patrón **más** su plano de tierra. El plano es parte del radiador. Copiar el patrón sobre un plano de forma o tamaño distinto la desafina. Por eso v0 replica también la proporción de la placa (50 × 80 mm) y no solo la antena.

---

## 5. Rendimiento medido de la referencia (UM §1.6, pág. 9)

Dos barridos de un E5071C, 700 MHz – 1.1 GHz, IFBW 70 kHz, fechados 2021-08-17 11:29/11:30.

| Marcador | f (MHz) | Return loss (S22) | VSWR |
|---|---|---|---|
| 1 | 863.00 | −8.0002 dB | 2.3271 |
| 2 | 865.00 | −8.2706 dB | 2.26?8 |
| 3 | 868.10 | −8.6878 dB | 2.1722 |
| **4** | **902.10** | **−16.428 dB** | **1.3729** |
| 5 | 921.60 | ≈ **−31.7 dB** ⚠ | 1.05?0 |
| 6 | 928.00 | −29.284 dB | 1.0498 |

### Sobre la legibilidad de estas cifras (R2)

Las dos gráficas son **capturas de pantalla en baja resolución embebidas en el PDF**: subir el render de 400 a 900 dpi no añade información, el bitmap de origen ya está agotado. Los marcadores 1, 3, 4 y 6 se leen sin duda. Los marcadores 2 y 5 tienen un dígito ambiguo cada uno.

**Reconciliación de los dos barridos** (RL y VSWR son la misma medida, así que deben ser consistentes — `|Γ| = 10^(RL/20)`, `VSWR = (1+|Γ|)/(1−|Γ|)`):

| f | RL leído | VSWR implicado | VSWR leído | ¿Coherente? |
|---|---|---|---|---|
| 863.0 | −8.0002 | 2.323 | 2.3271 | ✅ |
| 868.1 | −8.6878 | 2.163 | 2.1722 | ✅ |
| **902.1** | **−16.428** | **1.355** | **1.3729** | ✅ |
| 928.0 | −29.284 | 1.071 | 1.0498 | ✅ (dentro del error de lectura) |
| 921.6 | *«−17.6»* | *1.304* | **1.05x** | ❌ **incoherente** |

Un VSWR de 1.05x exige un RL de **≈ −31.4 dB**. La lectura correcta del marcador 5 es por tanto **−31.69 dB**, no −17.69: el primer dígito se lee mal en el bitmap.

### Corrección al Apéndice A de la guía

El Apéndice A afirma, como dato **verificado**, `921.60 MHz → −37.69 dB / VSWR 1.06`.

- El **−37.69 dB** es una **mala lectura de −31.69** (mismo dígito ambiguo, resuelto al revés). Un RL de −37.69 dB implicaría VSWR 1.026, que no es lo que dice la gráfica de VSWR.
- El **VSWR 1.06** está bien (el valor real es 1.05x).

**Más relevante: el Apéndice A concluye «la antena está afinada a ~921.6 MHz».** Eso no es lo que muestran los datos. El VSWR sigue bajando de 921.6 (1.05x) a 928.0 (1.0498), así que el mínimo está **por encima** de 921.6. **La resonancia de la referencia está en el rango ≈ 924–928 MHz.** No cambia ninguna decisión, pero es el número que F5 debe usar si intenta re-sintonizar hacia 902.2.

### Lo que sí está sólido, y es lo único que necesitamos

**A 902.10 MHz la antena de referencia mide −16.428 dB / VSWR 1.3729.** Es el único punto verificado por dos barridos independientes, y es la frecuencia de RC2.

**Criterio de aceptación para nuestra placa (F5/F7): ≤ −10 dB @ 902.2 MHz.**

Es un criterio deliberadamente holgado. La referencia consigue −16.4 dB en una placa de 50 × 80 mm con el plano exacto para el que se diseñó la antena; nuestra v0 replica esa proporción pero no será idéntica. −10 dB = 90 % de la potencia radiada = 0.46 dB de pérdida por desadaptación. Perseguir −16 dB en v0 sería optimizar antes de tener un uplink.

> **No confundir esto con un problema (trampa 6 de la guía).** Que la antena resuene ~25 MHz por encima de RC2 y aun así dé −16 dB en 902.1 es el comportamiento **esperado** de esta antena: es de banda ancha suficiente para cubrir 902–928. Al medir con el NanoVNA, ver el mínimo alrededor de 925 MHz **no** es un defecto de nuestra placa.

---

## 6. Pestaña desprendible (UM §1.9, pág. 12)

El §1.9 «EVB Radiation → Conduction Change» documenta el procedimiento de la referencia: **desprender la pestaña de la antena y pasar a alimentar por el SMA** para medir en conducido.

El dibujo de la pág. 8 muestra **5 ranuras de esquinas redondeadas** alineadas en el límite inferior de la pestaña (3 largas + 2 cortas), que son la línea de rotura. **No están acotadas en ninguna fuente.**

**Deuda para F5:** las ranuras hay que dimensionarlas nosotros contra las reglas de *tab routing / mouse bites* de JLCPCB, no copiarlas del dibujo. Y hay una decisión que F5 debe plantear: **¿v0 necesita la pestaña desprendible?**

- **A favor:** permite medir en conducido con u.FL, que es la única forma de separar «la antena está mal» de «la radio está mal» en el bring-up.
- **En contra:** una línea de ranuras a 5.60 mm del feed es una discontinuidad mecánica justo donde pasa la RF, y en v0 no se va a producir en volumen.

Los verticales del feed **cruzan la línea de ranuras**, así que las ranuras no pueden ir donde quieran: su posición está acoplada a la geometría del feed. F5 tiene que resolver ambas cosas a la vez.

### 6.1 Resolución (R4)

**La pestaña desprendible NO va en el producto.** Es una prestación de la placa de evaluación de SJI, no de la antena: la §1.9 lo dice como procedimiento de laboratorio — (1) *PCB ANT remove*, (2) *C101 remove*, (3) *CON101 RF SMA connector insertion* — y el esquemático encierra ANT1 y C101 en un recuadro rotulado `CUT`. En una cerradura, una antena diseñada para romperse a mano es un punto de fallo mecánico.

Cómo queda implementado, para que la decisión sea reversible sin tocar la antena:

- Las 5 ranuras viven en un **footprint aparte**, `ANT_LSM110A_BreakAwaySlots_EVM_ONLY`, cuyo nombre lleva el alcance. No están dentro del footprint de la antena.
- Sus cotas están **medidas** sobre el bitmap del UM a 8.203 px/mm (`SLOTROW_*` en `antenna_geometry.py`), con incertidumbre **±0.12 mm** — sirven para reproducir la EVM, no para fabricar. Si alguna vez hicieran falta en producción, hay que redimensionarlas contra las reglas de *tab routing* de JLCPCB, como pedía F1.
- El acoplamiento que F1 señala es real y está resuelto en la geometría: las ranuras `C` y `D` **esquivan** el feed (terminan 0.34 mm antes de `x = 33.50` y arrancan 0.43 mm después de `x = 34.50`), y `E` esquiva la pata del stub. Están en el módulo con esos huecos, no como una fila uniforme.

**Para el bring-up de v0 el argumento «a favor» sigue en pie, pero se cubre de otra forma:** la placa de prueba de `Hardware/KiCad/` lleva un **punto de test coaxial de 50 Ω** (`TP_Coax_50R_NanoVNA`) al pie de la línea RF. Eso da la medida en conducido con NanoVNA sin necesidad de romper nada.

---

## 7. Confirmación de tamaño — pregunta obligatoria de F5

La pestaña de antena sola mide **50.00 × 20.03 mm**. La placa de referencia completa es **50 × 80 mm** (FCC pág. 5).

Esto **rompe el objetivo de <5 cm** del producto. Es exactamente el sacrificio que v0 acordó, pero **F5 tiene que reconfirmarlo con Franco antes de rutear** (§5-F5 de la guía). Ancho de 50 mm significa que el producto v0 no cabe en la carcasa final; v0 es una placa de validación de radio, no un prototipo de forma.
