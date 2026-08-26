# Verificación

El diseño se verifica ejecutando `tools/build.sh`, que regenera todo desde
`antenna_geometry.py` y corre **122 comprobaciones**. Devuelve código ≠ 0 si algo falla,
así que se puede colgar de CI tal cual.

```
cd Hardware/KiCad/tools && ./build.sh
```

Requisito: KiCad ≥ 7 con los bindings de Python (`apt install kicad` en Debian/Ubuntu,
o el `pcbnew` de la instalación oficial).

## Resultado actual

| Bloque | Comprobaciones | Estado |
|---|---|---|
| Cadena de cotas | 17 | ✅ |
| Footprints, cargados con el motor de KiCad | 63 | ✅ |
| Placa: DRC + cotas del cobre real | 42 | ✅ |
| **Total** | **122** | **✅ 0 fallos** |

Además, una verificación **píxel a píxel contra el plano oficial** del User Manual
LSM110A Rev 1.4 (sección 1.5): los seis niveles verticales, las dos aperturas de ranura, los
extremos horizontales y hasta el gap del CPWG caen donde los pone esta reconstrucción. Ver
[`geometria-antena.md`](geometria-antena.md#verificación-contra-el-plano-oficial).

DRC de la placa de prueba: **0 violaciones, 0 pads sin conectar, 0 errores de footprint**
(`export/drc-report.txt`).

## Cómo se verifica, y por qué así

La idea de fondo: **medir el resultado, no confiar en la intención.** Escribir un archivo
de KiCad no prueba nada; lo que prueba algo es cargarlo con el parser de KiCad y medir la
geometría que sale.

### 1. Cadena de cotas (`antenna_geometry.py`)

Comprueba que las cotas del plano son mutuamente consistentes. Lo importante:

- `4.00 + 13.00 + 3.03 == 20.03` — la cadena vertical cierra con la cota del plano.
- `39.50 − 34.00 == 5.50` y `39.50 − 6.00 == 33.50` — los puentes salen de las ranuras.
- `área de rectángulos == área del contorno por fórmula del zapatero`.
- El contorno es rectilíneo: todos los tramos horizontales o verticales, sin vértices
  repetidos.

Esas dos últimas encontraron un error real: en la primera versión el contorno tenía un
vértice de menos y cruzaba en diagonal la antena. Las áreas no coincidieron y saltó.

### 2. Footprints (`verify.py`, vía `pcbnew`)

Carga los `.kicad_mod` con `IO_MGR.PluginFind(KICAD_SEXP).FootprintLoad()` — el mismo
parser de KiCad — y mide sobre el `SHAPE_POLY_SET` efectivo del pad:

- **`OutlineCount() == 1` y `HoleCount(0) == 0`.** Esta es la comprobación clave: prueba
  que el cobre es una sola pieza conexa sin huecos, o sea que los dos puentes unen los
  tres brazos y que ninguna ranura parte la antena en dos. Es la topología en «2»
  verificada por el motor de geometría de KiCad, no por inspección visual.
- **16 sondas punto a punto**: 8 sitios donde tiene que haber cobre (centro de cada brazo,
  los dos puentes, los dos tramos del stub, la línea de feed) y 8 donde no (dentro de cada
  ranura, en sus extremos abiertos, el hueco entre feed y stub, fuera de la antena).
- Área de cobre y bounding box contra los valores del plano.
- **Net tie**: `fp.IsNetTie()` y `fp.GetNetTiePads(pad1) == ['1','2']`, o sea que KiCad
  reconoce que el corto entre feed y GND es intencional.
- **Máscara**: los pads están solo en `F.Cu`, sin `F.Mask` ni `F.Paste`. El cobre queda
  cubierto por la máscara de soldadura, no expuesto.
- **Rule area**: prohíbe plano, vías y pistas; **permite** pads (si no, marcaría el cobre
  de la propia antena); presente en `F.Cu` y `B.Cu`; sus cuatro bordes en su sitio.
- La fila de troquelado del EVM: 5 × 4 tramos en `Edge.Cuts` y ≥ 0.25 mm de holgura al
  cobre del feed y de la pata del stub. El umbral es 0.25 y no más porque las cotas
  **medidas** del plano dejan 0.28 mm entre la ranura D y la pata del stub: es una línea
  pensada para romper, no para respetar holguras generosas.

### 3. Placa (`verify_board.py`)

Corre el DRC con `pcbnew.WriteDRCReport()` y después mide el **cobre ya rellenado**:

- **Dónde arranca el plano de tierra.** La cota más crítica del diseño. Se mide el
  bounding box del relleno en las dos capas: **20.03 mm exactos**. Además se barre el área
  de la antena en una rejilla de 0.5 mm buscando puntos de plano que se hayan colado:
  **0 intrusos** en `F.Cu` y en `B.Cu`.
- **Gap coplanar real del CPWG.** Se barre en X desde el borde de la pista con paso de
  1 µm hasta encontrar el plano: **0.151 mm** a los dos lados (el 0.001 es el paso del
  barrido). Es un parámetro de impedancia, no una holgura de fabricación, así que se mide
  en lugar de suponerse.
- Cota 5.60: borde del pad del componente serie (`L101`) a 25.63 = 20.03 + 5.60.
- Rotación de `L101` (la posición serie): pad 1 hacia la antena (`ANT_FEED`) y arriba,
  pad 2 hacia el radio (`RF_IN`). Esta comprobación existe porque la primera versión los
  tenía al revés — `SetOrientationDegrees(90)` los intercambia.
- **Los condensadores shunt montan sobre la línea.** Para `C101` y `C102` se mide el
  solape entre su pad interior y la pista RF: 0.45 mm cada uno. Si el pad no monta sobre
  la línea, el condensador queda colgando de un muñón y su inductancia serie arruina
  justo lo que tiene que hacer. Y se comprueba que cada pad de GND tenga una vía de
  retorno pegada (0.00 mm de holgura), por lo mismo.
- Contorno 50 × 80 medido sobre la **línea media** de `Edge.Cuts` (el bounding box incluye
  el ancho de línea; descontarlo o sale 50.10).
- 47 vías, todas en GND, **0 dentro del área de la antena**. La costura general
  esquiva los footprints del matching y de J1: una vía encima de un pad de GND no daría
  error de DRC (misma red), quedaría sin marcar y mal.
- La pista RF arranca fuera del keepout **contando su casquete redondo** de 0.50 mm.

## Errores que encontró esta verificación

Se listan porque son la razón de que el proceso valga la pena.

| # | Error | Cómo apareció |
|---|---|---|
| 1 | Vértice de menos en el contorno → diagonal cruzando la antena | render SVG + área del zapatero ≠ suma de rectángulos |
| 2 | `SetOrientationDegrees(90)` intercambia los pads de C101 → la línea RF llegaba al pad equivocado | DRC: *solder mask bridge* + *unconnected items* |
| 3 | Los casquetes redondos de dos pistas de 1.00 mm se tocan sobre un paso de pads de 0.97 mm | DRC: clearance 0.0000 mm entre las dos pistas RF |
| 4 | La pista RF arrancaba justo en el borde del keepout → contaba como dentro | DRC: *items not allowed (keepout ANT_KEEPOUT)* |
| 5 | Serigrafía de placa de 51 mm de ancho en una placa de 50 mm | DRC: *silkscreen clipped by board edge* |
| 6 | Texto de serigrafía de 0.7 mm, por debajo del mínimo de 0.8 | DRC: *text height out of range* |
| 7 | FPID sin nickname de biblioteca | DRC: *the current configuration does not include the library ''* |
| 8 | Serigrafías de `C102` y `J1` solapadas al añadir la red en π | DRC: *silkscreen overlap* |
| 9 | El punto de sondeo del gap coplanar caía sobre un pad de `C101` y medía 0.276 en vez de 0.15 | el propio `verify_board.py`, al añadir los shunts |

El DRC bajó de **16 violaciones a 0**.

### Y tres rarezas de la API de KiCad 7, por si vuelven a morder

1. `ZONE_FILLER.Fill()` **segfaultea** sobre un `pcbnew.BOARD()` suelto: necesita un
   `PROJECT`, que solo aparece pasando por `pcbnew.LoadBoard()`. De ahí que el generador
   guarde, recargue y luego rellene.
2. `SaveBoard()` **reescribe el `.kicad_pro`** desde las `NET_SETTINGS` en memoria. Un
   `.kicad_pro` escrito a mano antes del primer `SaveBoard` se pierde entero.
3. Dentro de un mismo proceso, `LoadBoard()` **reutiliza el `PROJECT` cacheado** en vez de
   releer el `.kicad_pro` del disco. Por eso la netclase por defecto se fija por API y la
   netclase `RF` (que KiCad 7 no expone por Python) se parchea al final, cuando ya no
   queda ningún guardado que la pise.

`FOOTPRINT::Add()` tampoco acepta `ZONE` ni `PCB_SHAPE` en KiCad 7 (solo `FP_SHAPE`), así
que los `.kicad_mod` se escriben a mano. La sintaxis canónica se extrajo haciendo que
`pcbnew.IO_MGR` guardara footprints y placas de prueba, y copiando su formato exacto.

## Lo que la verificación NO cubre

Importante tenerlo claro:

- **La frecuencia de resonancia de la placa fabricada.** Nada de esto la predice. Lo que
  sí hay ahora es contra qué compararla: el S11 medido de la referencia (arriba).
- **Que el redibujo coincida con el Gerber oficial de SJI.** Se verificó contra el plano
  cotado del User Manual, píxel a píxel, y coincide en todos los rasgos medibles. Lo que el
  bitmap del manual no resuelve es el detalle por debajo de ~0.12 mm: radios de esquina,
  compensaciones de grabado. Ver el aviso de certificación en el README.
- **La impedancia real del CPWG.** Los 50 Ω vienen del User Manual FCC para 1.00 / 0.15 /
  FR4 1.6 mm / εr 4.3. La geometría se verifica; el valor de impedancia se hereda de esa
  fuente y depende del stack-up real del fabricante.
- **Las cotas de la fila de ranuras opcional**, que no están cotadas en el plano.

## Criterio de aceptación de la medida

La sección 1.6 del User Manual publica el S11 y el VSWR **medidos** de esta antena, con la
población de referencia sobre la PCB de 50 × 80 mm. Eso convierte el ajuste en un
pass/fail cuantitativo en lugar de un «parece que va bien»:

| Frecuencia | S11 de referencia (SJI) | Objetivo en la placa fabricada |
|---|---|---|
| 902.1 MHz | −16.43 dB | ≤ −10 dB, y a menos de ~3 dB de la referencia |
| 921.6 MHz | −17.6 dB | ídem |
| 928.0 MHz | −29.28 dB | ídem |
| 868.1 MHz | −8.69 dB | *no* es banda de trabajo; sirve de comprobación de forma |

Si la placa fabricada reproduce esa curva, el redibujo queda **validado eléctricamente**, no
solo geométricamente. Si la resonancia sale desplazada, la diferencia apunta a la geometría
o al stack-up del fabricante (εr real, espesor), no al método.

Ojo con dos cosas al comparar:

- La referencia se midió en el nodo **RFOUT**, con `L101` = 0 Ω, `C101` = 2.2 pF y
  `C102` sin poblar. Hay que medir con esa misma población.
- La referencia no lleva los ~10 mm de línea RF que esta placa mete entre la antena y `J1`.
  Son ≈ 27° a 915 MHz: hay que descontarlos o dejarlos anotados como offset.

## Procedimiento de ajuste con NanoVNA

La placa `antenna-test-board` existe para esto. Lleva plano de tierra de 50 × 80 mm
(la PCB de referencia de SJI del expediente FCC) porque **en una IFA el plano de tierra
es el contrapeso y forma parte de la antena**: medir sobre otro plano da otro resultado.

1. Soldar un pigtail coaxial de 50 Ω (RG-178 o semirrígido) en **J1**. Malla a los pads
   1 y 3, vivo al pad 2.
2. Calibrar el NanoVNA (open/short/load) **en el extremo del pigtail**, no en el puerto
   del instrumento.
3. Poblar C101 y medir S11 de 800 a 1000 MHz. La línea RF entre la antena y J1 mide
   ≈ 10 mm (≈ 27° a 915 MHz): descontarla o dejarla registrada como offset.
4. Ajustar con la **red en π**, que es el grado de libertad rápido y no toca el cobre.
   Las tres posiciones están montadas: `L101` en serie, `C101` shunt del lado antena,
   `C102` shunt del lado radio. Orden recomendado, con un kit de 0402:

   | Paso | Población | Para qué |
   |---|---|---|
   | 1 | `L101` = 0 Ω, `C101` = 2.2 pF, `C102` = DNI | la de referencia: punto de partida |
   | 2 | barrer `C101` de 0.5 a 5.6 pF | mueve la resonancia y la parte reactiva |
   | 3 | si queda reactancia inductiva, `L101` → condensador serie (0.5 – 10 pF) | L en serie |
   | 4 | si queda capacitiva, `L101` → inductor (2 – 22 nH) | corrige al otro lado |
   | 5 | si la parte real no cuadra, poblar `C102` (π completa) | transforma la impedancia |

   Con una IFA lo normal es que baste el paso 2 o el 3. La π completa se reserva para
   cuando la antena está bien resonada pero la impedancia no es 50 Ω.

   Ojo con una cosa: al medir con el NanoVNA en J1 se está viendo **la antena más la red**.
   Para saber qué hace la antena sola, quitar `C101`/`C102` y poner 0 Ω en `L101`.

5. Si con la red no llega (la antena resuena demasiado lejos):
   - resuena **alto** → alargar el recorrido de corriente (alargar las ranuras, o sea
     reducir los puentes de 5.50 / 6.00),
   - resuena **bajo** → acortarlo (ensanchar los puentes).

   Los puentes están parametrizados: se cambian `SLOT1_LEN` y `BRIDGE_L` en
   `antenna_geometry.py` y se regenera todo con `build.sh`.
6. Objetivo: S11 ≤ −10 dB en toda la banda 902 – 928 MHz.
7. **Volver a medir con la caja cerrada y la CR2450 montada.** Ambas desintonizan una IFA,
   y la pila es un plano metálico. El ajuste final se hace sobre el producto ensamblado,
   nunca sobre la placa desnuda.

## Regenerar

Todo sale de `antenna_geometry.py`. Para cambiar una cota, se cambia ahí y se corre
`build.sh`.

No editar los `.kicad_mod`, el `.kicad_sym` ni el `.kicad_pcb` a mano: el siguiente
`build.sh` los sobrescribe.

### El build es reproducible byte a byte

Regenerar sin cambiar la geometría **no produce ningún diff en git**. Así, cuando el diff
de un fichero generado muestra algo, ese algo es un cambio real de geometría. Conseguirlo
costó tres cosas, porque pcbnew no lo da gratis:

1. **Footprints y símbolo.** UUID deterministas (`uuid5` con namespace propio del
   proyecto), asignados al generar el texto. Directo.

2. **UUID de la placa.** pcbnew asigna un KIID aleatorio a cada item y `m_Uuid` es de solo
   lectura desde Python, así que no se pueden fijar al construir. Se sustituyen a
   posteriori sobre el texto guardado, por orden de primera aparición. Todas las
   apariciones de un mismo UUID van al mismo valor, así que las referencias cruzadas del
   archivo siguen siendo válidas.

3. **Orden de los bloques.** Al guardar, KiCad ordena footprints, pistas, vías y zonas con
   un criterio que acaba dependiendo de esos KIID aleatorios — y reordena también los
   hijos *dentro* de cada footprint. Sin tocar nada, el orden de los bloques cambiaba en
   cada ejecución. `canonicalize()` reordena los dos niveles por *(tipo, texto sin UUID)*,
   que no depende de nada aleatorio, preservando la indentación. El orden de los hijos de
   una s-expresión no tiene significado semántico, así que reordenarlos es seguro; y de
   todos modos `verify_board.py` vuelve a cargar el archivo y corre el DRC justo después.

Los SVG exportados y el informe DRC llevan la fecha de generación, que se normaliza
también. La fecha real de un artefacto generado es la de su commit.

Comprobado ejecutando `build.sh` tres veces seguidas: mismo MD5 del `.kicad_pcb` las tres.
