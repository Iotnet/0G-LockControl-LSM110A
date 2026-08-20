# Validación del footprint del repo — medido, no leído

**Fase:** F1, punto 7 · **Fecha:** 2026-08-20
**Archivo validado:** `Hardware/EasyEDA/Footprints/PCB_PCB_LSM_Module_2026-06-04.json`
**Referencia:** DS R08 **Fig. 5-3-1 «Recommended footprint (top view)» (pág. 16)** y **Fig. 5-2-1 «Outer Dimensions» (pág. 16)**
**Método (R5):** JSON parseado; cada pad extraído del bloque `LIB~` (34 entradas `PAD~RECT~`); medidas calculadas de las coordenadas, no leídas del README.

---

## 1. Escala del archivo — hay que fijarla antes de medir

Las coordenadas del JSON de EasyEDA son adimensionales. La escala se determinó por consistencia interna, probando las dos candidatas habituales:

| Escala supuesta | Pitch resultante | Tamaño de pad | Vano pin 1→12 | ¿Coherente con la Fig. 5-3-1? |
|---|---|---|---|---|
| 1 u = 0.25 mm | 0.9842 mm | 1.1811 × 0.5906 mm | 10.8267 mm | ❌ nada cuadra |
| **1 u = 10 mil = 0.254 mm** | **1.0000 mm** | **1.2000 × 0.6000 mm** | **11.0000 mm** | ✅ **todo exacto** |

Con `1 u = 0.254 mm` **los cuatro valores salen exactos a cuatro decimales**. Eso no es coincidencia: fija la escala sin ambigüedad. Todas las medidas siguientes usan ese factor.

---

## 2. Resultados

| Parámetro | DS Fig. 5-3-1 | **Medido en el JSON** | Δ |
|---|---|---|---|
| Nº de pads | 34 | **34** | ✅ |
| Distribución | 12 izq · 10 abajo · 12 der · 0 arriba | **12 · 10 · 12 · 0** | ✅ |
| Numeración col. izq. | 1…12 (arriba→abajo) | **1…12** | ✅ |
| Numeración fila inf. | 13…22 (izq→der) | **13…22** | ✅ |
| Numeración col. der. | 23…34 (abajo→arriba) | **23…34** | ✅ |
| Pitch | 1.00 mm | **1.0000 mm** | **0.0000** |
| Tamaño de pad | 1.20 × 0.60 mm | **1.2000 × 0.6000 mm** | **0.0000** |
| Vano col. izq. (1→12) | 11.00 mm | **11.0000 mm** | **0.0000** |
| Vano col. der. (23→34) | 11.00 mm | **11.0000 mm** | **0.0000** |
| Vano fila inf. (13→22) | 9.00 mm | **9.0000 mm** | **0.0000** |
| Gap entre pads adyacentes | 0.40 mm | **0.4000 mm** (los 9 de la fila inferior) | **0.0000** |
| Ancho de contorno (serigrafía) | 14.00 mm | **14.0000 mm** | **0.0000** |
| Separación entre columnas | *(no acotada)* | 13.1999 mm centro-centro | — |
| **Pad central** | **no existe** | **no existe** | ✅ |

### Orientación de los pads — se verificó, y está bien

Los campos `w`/`h` de la cabecera de cada `PAD~` dicen `1.20 × 0.60` para **todos** los pads, incluidos los de la fila inferior, y `rot = 0`. Eso parecía un error grave: con pitch de 1.00 mm, pads de 1.20 mm de ancho se solaparían y **cortocircuitarían los pines 13 a 22**.

**No es un error.** Al medir el **polígono real** de cada pad (campo 11 del registro, la lista de vértices), la fila inferior sale **0.600 × 1.200 mm** — correctamente girada 90°. El gap borde-a-borde medido es de **0.400 mm** en los nueve intervalos, exactamente lo de la Fig. 5-3-1.

> **Lección de método:** los campos `w`/`h` de la cabecera de EasyEDA **no reflejan la rotación**; hay que medir el polígono. Validar por la cabecera habría producido una alarma falsa de cortocircuito, y validar «a ojo» sobre la captura de pantalla no habría medido nada. Aplica también a la revisión de lo que Franco exporte en F3/F6.

---

## 3. Única desviación numérica encontrada

Comprobación de las tres filas/columnas contra una retícula ideal de 1.000 mm:

```
Columna izquierda (1…12):   pin 2 = +0.0160 mm   ← única desviación
                            el resto = 0.0000
Columna derecha (23…34):    todos = 0.0000
Fila inferior (13…22):      todos = 0.0000
```

**El pad 2 está 0.016 mm desplazado.** Consecuencia: `pin1→pin2 = 1.016 mm` y `pin2→pin3 = 0.984 mm`, en lugar de 1.000 mm. El vano total 1→12 sigue siendo **11.0000 mm exacto**, así que el error no se acumula ni desplaza el resto de la columna.

**Veredicto: irrelevante para fabricación.** 0.016 mm = 0.63 mil, contra un gap nominal de 0.400 mm y tolerancias de JLCPCB dos órdenes de magnitud mayores. Es un artefacto de edición manual, no un defecto.

**Recomendación:** corregirlo de todas formas, porque cuesta un arrastre de pad y elimina la única cifra no exacta del archivo. Prioridad baja; que no bloquee F3.

### Asimetría del contorno de serigrafía

Los cuatro trazos de esquina de la serigrafía dan **ancho 14.0000 mm exacto**. En vertical:

- del centro del pad 12 al trazo inferior: **2.0000 mm** (exacto)
- del centro del pad 1 al trazo superior: **2.0820 mm** (+0.082 mm)
- alto total resultante: **15.082 mm** frente a los 15.00 mm de la Fig. 5-2-1

**El trazo superior de serigrafía está 0.082 mm demasiado alto.** Es serigrafía, no cobre: no afecta a la soldabilidad ni al DRC. Corregir cuando se toque el archivo.

---

## 4. Error en el README del footprint — corregir

`Hardware/EasyEDA/Footprints/README.md` afirma:

| Línea del README | Realidad |
|---|---|
| `Pad central (GND) \| Si (exposed pad)` | ❌ **No existe.** El JSON tiene 34 pads y ninguno central. |
| *«El pad central (exposed pad) es GND y debe conectarse al plano de tierra con vias térmicas»* | ❌ Instrucción sobre algo inexistente. |

La **Tabla 5-1-1 del DS R08 no lista ningún pad central**, la **Fig. 5-3-1 no lo dibuja**, y el **JSON no lo tiene**. Las tres fuentes coinciden: no hay pad central, y no debe haberlo.

**Por qué importa y no es solo un texto mal:** si alguien sigue el README y añade un plano de cobre con vías térmicas bajo el módulo, mete cobre en la cara superior justo debajo del encapsulado — donde el DS §5.4 (pág. 17) pide precisamente lo contrario: **recubrimiento de tinta PSR** para evitar cortocircuitos y descargas contra la parte inferior del módulo. El README no solo describe algo que no existe: **empuja al error opuesto al que el fabricante advierte**. Esto conecta con **H-09** (falta el PSR coating), que resuelve F6.

**Acción:** eliminar esas dos líneas del README y sustituirlas por una nota explícita de «sin pad central; PSR coating bajo el módulo según DS §5.4».

---

## 5. Veredicto

**El footprint del repo es correcto y se puede usar en v0 sin cambios.**

- Todas las cotas críticas (pitch, tamaño de pad, gaps, los tres vanos, ancho del contorno) coinciden con la Fig. 5-3-1 **de forma exacta**.
- La única desviación en cobre es de **0.016 mm en un pad**, 25 veces menor que cualquier tolerancia relevante.
- La única desviación en serigrafía es de **0.082 mm**, sin efecto funcional.
- **No hay pad central**, que es lo correcto. Lo que hay que corregir es el README, no el footprint.

Esto confirma —y afina con números— la afirmación del Apéndice A de la guía («coincide dentro de 0.02 mm»): el número exacto es **0.016 mm, y en un solo pad**.

---

## Reproducir estas medidas

```python
import json
U = 0.254  # 1 unidad EasyEDA = 10 mil
d = json.load(open('PCB_PCB_LSM_Module_2026-06-04.json'))
lib = [s for s in d['shape'] if s.startswith('LIB~')][0]
for p in lib.split('#@$'):
    if not p.startswith('PAD~'): continue
    f = p.split('~')
    n = int(f[8])
    poly = [float(v) for v in f[10].split()]      # <-- el polígono, NO f[4]/f[5]
    xs, ys = poly[0::2], poly[1::2]
    print(n, (max(xs)-min(xs))*U, (max(ys)-min(ys))*U)
```
