# Antena de referencia — cotas, keepout y rendimiento medido

**Fase:** F1 · **Fecha:** 2026-08-20
**Fuentes:** UM Rev1.4 **§1.5 (pág. 8)** — cotas · **§1.6 (pág. 9)** — return loss y VSWR medidos · **§1.9 (pág. 12)** — pestaña · FCC **pág. 5** — renders top/bottom
**Método (R3):** pág. 8 renderizada a **600 dpi** y recortada con PIL; zona del feed reescalada ×1.5 para leer el cúmulo de cotas pequeñas. `pdftotext` de la pág. 8 devuelve solo `[ Antenna Pattern ]` y `[ Matching ]` — **toda la ingeniería está en el dibujo.**

---

## 1. Advertencia de alcance

**F1 registra las cotas tal como el dibujo las da. No reconstruye la geometría.**

El dibujo de la pág. 8 es un plano acotado, no una lista de coordenadas: da longitudes y anchos, pero **no da el origen ni todas las cotas necesarias para cerrar el polígono de forma única**. Reconstruir la IFA como lista de segmentos con coordenadas es trabajo de **F5**, y F5 va a tener que resolver ambigüedades — están señaladas en §3.

Cualquier cota de este documento que F5 use para generar cobre debe re-verificarse contra el render a 600 dpi. La lista de abajo es fiel a lo legible; la interpretación geométrica no está verificada.

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
| Brazo 1 (superior) | **39.50** | longitud horizontal del trazo más alto | alta |
| Brazo 2 | **34.00** | longitud horizontal del segundo trazo | alta |
| Separación brazo 1 ↔ brazo 2 | **5.00** | vertical, medida en el extremo izquierdo | media |
| Separación brazo 2 ↔ brazo 3 | **5.00** | vertical, medida en el centro | media |
| Caída izquierda | **7.50** | vertical, del brazo 2 hacia abajo | media |
| Caída derecha | **10.50** | vertical, del brazo 1 hacia la zona del feed | media |
| Retorno derecho | **5.00** | horizontal, del feed al borde derecho | media |
| — | **2.00** | vertical, cerca del extremo izquierdo del brazo 3 | **baja** |
| — | **3.03** | vertical, zona inferior central | **baja** |
| — | **4.05** | vertical, lado derecho, zona de las ranuras | media |
| — | **5.60** | vertical, del borde inferior de la pestaña al pad de C101 | alta |

### 2.3 Anchos de pista

| Ancho | Valor | Dónde |
|---|---|---|
| Pista radiante | **0.50** | brazos meandreados — la cota `0.50` aparece dos veces, en el brazo 2 y en el brazo 3 |
| Feed / stub | **1.00** | los dos verticales que bajan por la pestaña, y el retorno del borde derecho (`1.00` aparece tres veces) |

**La pista radiante es de 0.50 mm y el feed de 1.00 mm.** Son distintos: no uniformar.

### 2.4 Zona del feed (recorte ampliado)

Estructura legible: **dos verticales de 1.00 mm de ancho** bajan desde el cuerpo de la antena y cruzan la banda de las ranuras de desprendimiento:

- el **vertical izquierdo** termina en el pad de **C101** (rotulado en el dibujo), a **5.60 mm** por debajo del borde de la pestaña;
- el **vertical derecho** continúa hacia abajo y es el que lleva la flecha **`Signal Input`** — es la entrada de los 50 Ω que vienen del módulo;
- un **tercer vertical de 1.00 mm** corre por el borde derecho, separado **5.00 mm** del par anterior.

`C101` es el 2.2 pF de `red-rf.md` §1 (shunt a GND del lado de la antena) y **está físicamente al pie de la antena, no junto al módulo**. Dato de layout para F5/F6: el condensador de sintonía va abajo, en la placa, inmediatamente bajo la línea de corte de la pestaña.

---

## 3. Lo que el dibujo **no** cierra (deuda para F5)

1. **Cuál de los tres verticales es el feed y cuál el cortocircuito a masa.** Una IFA tiene un feed y un stub de cortocircuito a tierra. La flecha `Signal Input` identifica el feed; el papel de los otros dos (uno va a C101, otro corre por el borde) **no está rotulado**. Es la ambigüedad más importante y la que más afecta a la sintonía.
2. **Origen de coordenadas.** No hay datum. F5 debe fijar uno (propuesta: esquina inferior-izquierda de la pestaña) y declararlo.
3. **Las cotas `2.00` y `3.03`.** Ambas caen sobre trazos superpuestos; la confianza es baja incluso a 600 dpi. Re-leer antes de usarlas.
4. **Cierre dimensional.** Las verticales legibles (`5.00 + 5.00 + 7.50`, o `10.50 + 4.05`) no suman `20.03` de forma obvia. Puede ser que unas midan centro-a-centro y otras borde-a-borde. **F5 debe reconciliarlo aritméticamente antes de generar cobre** — si las verticales no cierran contra los 20.03, la geometría está mal interpretada.
5. **Geometría de las ranuras de desprendimiento.** Se ven **5 ranuras** de esquinas redondeadas en el dibujo (3 largas + 2 cortas), pero no están acotadas. Ver §6.

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

Los tres verticales del feed **cruzan la línea de ranuras**, así que las ranuras no pueden ir donde quieran: su posición está acoplada a la geometría del feed. F5 tiene que resolver ambas cosas a la vez.

---

## 7. Confirmación de tamaño — pregunta obligatoria de F5

La pestaña de antena sola mide **50.00 × 20.03 mm**. La placa de referencia completa es **50 × 80 mm** (FCC pág. 5).

Esto **rompe el objetivo de <5 cm** del producto. Es exactamente el sacrificio que v0 acordó, pero **F5 tiene que reconfirmarlo con Franco antes de rutear** (§5-F5 de la guía). Ancho de 50 mm significa que el producto v0 no cabe en la carcasa final; v0 es una placa de validación de radio, no un prototipo de forma.
