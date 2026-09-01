"""
Geometria parametrica de la antena IFA ranurada del LSM110A ("1.5 Antenna Dimension").

FUENTE DE VERDAD del diseno: todo el resto (footprint, simbolo, placa de prueba,
render SVG) se genera a partir de este modulo. Si hay que corregir una cota, se
corrige aqui y se regenera todo con `tools/build.sh`.

Sistema de coordenadas
----------------------
Origen  = esquina superior izquierda del cobre de la antena.
+X      = hacia la derecha.
+Y      = hacia ABAJO (convencion KiCad, no matematica).
Unidades = milimetros.

Con este origen, el borde superior de la PCB queda en y = -EDGE_TO_ANT_TOP y el
borde izquierdo en x = -SIDE_MARGIN.

Cadena de cotas vertical (del plano)
-----------------------------------
    4.00   borde superior PCB  -> borde superior antena
   13.00   alto del cuerpo de la antena  (5.00 + 0.50 + 5.00 + 0.50 + 2.00)
    3.03   borde inferior antena -> borde del plano de tierra
   ------
   20.03   total  (coincide exactamente con la cota 20.03 del plano)
"""

from __future__ import annotations

# --------------------------------------------------------------------------
# Donde se publican las bibliotecas KiCad  (no son cotas)
# --------------------------------------------------------------------------
# El proyecto tiene UNA sola casa de bibliotecas: la misma carpeta donde vive el
# LSM110A. Asi KiCad necesita un unico path registrado y el LSM110A, la antena y
# los componentes RF salen todos bajo el mismo nickname de footprints.
#
# Se define aqui, y no en cada generador, porque son cuatro scripts los que
# tienen que coincidir: si la ruta se repitiera en cada uno, tarde o temprano
# uno escribiria en un sitio y otro leeria de otro.
#
# Ruta relativa a la carpeta tools/. Verificado: KiCad enumera footprints en una
# carpeta PLANA, sin necesidad de que se llame ".pretty".

LIB_DIR = "../../v0-replica-sji/kicad-lib"
FP_LIB_NICKNAME = "0G-LockControl"  # footprints: la carpeta kicad-lib entera
SYM_LIB_NICKNAME = "0G-Antenna"     # simbolos: el archivo 0G_Antenna.kicad_sym
SYM_LIB_FILE = "0G_Antenna.kicad_sym"


def lib_dir(script_file: str):
    """
    Carpeta de bibliotecas, resuelta contra la ubicacion del script y no contra
    el directorio de trabajo: asi los generadores dan el mismo resultado se
    llamen desde tools/, desde la raiz del repo o desde CI.
    """
    from pathlib import Path
    return (Path(script_file).resolve().parent / LIB_DIR).resolve()


# --------------------------------------------------------------------------
# Cotas tomadas del plano "1.5 Antenna Dimension"
# --------------------------------------------------------------------------

BOARD_W = 50.00  # ancho total de la PCB de referencia
ANT_W = 39.50  # ancho del cobre de la antena
SIDE_MARGIN = (BOARD_W - ANT_W) / 2.0  # 5.25 -> antena centrada en la PCB

EDGE_TO_ANT_TOP = 4.00  # borde superior PCB -> borde superior antena
EDGE_TO_GND = 20.03  # borde superior PCB -> borde del plano de tierra
ANT_TO_GND = 3.03  # hueco antena -> plano de tierra

ARM_TOP_H = 5.00  # alto del brazo superior
ARM_MID_H = 5.00  # alto del brazo central
ARM_BOT_H = 2.00  # alto del brazo inferior
SLOT_W = 0.50  # ancho de ranura (nota 2 del plano)

SLOT1_LEN = 34.00  # ranura 1: abierta a la IZQUIERDA, largo 34.00
BRIDGE_L = 6.00  # ranura 2: puente IZQUIERDO de 6.00 (abierta a la derecha)

FEED_W = 1.00  # ancho de la linea de alimentacion (nota 3)
FEED_CENTER_X = SLOT1_LEN  # 34.00: la linea de feed va centrada en el fin de la ranura 1

STUB_W = 1.00  # ancho del stub en L (nota 4)
STUB_RUN = 5.00  # tramo horizontal del stub en L
CPWG_LEN = 5.60  # borde del plano -> pad de C101 (tramo CPWG 50 ohm)
CPWG_GAP = 0.15  # clearance coplanar del CPWG (User Manual FCC)

# --------------------------------------------------------------------------
# Cotas derivadas
# --------------------------------------------------------------------------

ANT_H = ARM_TOP_H + SLOT_W + ARM_MID_H + SLOT_W + ARM_BOT_H  # 13.00
BRIDGE_R = ANT_W - SLOT1_LEN  # 5.50  puente derecho
SLOT2_LEN = ANT_W - BRIDGE_L  # 33.50 largo de la ranura 2

# Niveles en Y (de arriba hacia abajo)
Y_TOP = 0.00  # borde superior de la antena
Y_SLOT1_T = ARM_TOP_H  # 5.00
Y_SLOT1_B = Y_SLOT1_T + SLOT_W  # 5.50
Y_SLOT2_T = Y_SLOT1_B + ARM_MID_H  # 10.50
Y_SLOT2_B = Y_SLOT2_T + SLOT_W  # 11.00
Y_BOT = Y_SLOT2_B + ARM_BOT_H  # 13.00  borde inferior de la antena
Y_GND = Y_BOT + ANT_TO_GND  # 16.03  borde del plano de tierra

# Linea de alimentacion: 1.00 de ancho, centrada en x = 34.00
FEED_X0 = FEED_CENTER_X - FEED_W / 2.0  # 33.50
FEED_X1 = FEED_CENTER_X + FEED_W / 2.0  # 34.50

# El brazo inferior termina alineado con el borde derecho del feed
ARM_BOT_X1 = FEED_X1  # 34.50

# Stub en L: sale del borde inferior del brazo inferior, corre 5.00 a la derecha
# y baja hasta el plano de tierra.
#
# El techo del tramo horizontal lo fija la cota 4.05 del plano, NO una suposicion.
# Las dos cotas verticales de esa zona comparten su flecha inferior (el borde del
# plano de tierra), asi que restarlas da el alto del tramo horizontal:
#
#       4.05  (techo del tramo horizontal -> borde del plano)
#     - 3.03  (fondo del cobre de la antena -> borde del plano)
#     = 1.02   alto del tramo horizontal
#
# Ese 1.02 NO esta acotado en el plano: es la unica cota de la cadena que el
# dibujo deja libre, y por eso es la que absorbe la diferencia. Una version
# anterior suponia que el tramo medida 1.00 (igual que el ancho de la pata), lo
# que daba 4.03 y obligaba a comprobar la cota 4.05 con tolerancia 0.03. Ahora
# la cota manda y la comprobacion es exacta.
STUB_V_EXT = 4.05  # cota del plano: techo del tramo horizontal -> borde del plano
STUB_TOP_Y = Y_GND - STUB_V_EXT  # 11.98
STUB_RUN_H = Y_BOT - STUB_TOP_Y  # 1.02  derivado de 4.05 - 3.03 (no acotado)
STUB_X0 = ARM_BOT_X1  # 34.50
STUB_X1 = STUB_X0 + STUB_RUN  # 39.50 (coincide con el borde derecho de la antena)
STUB_LEG_X0 = STUB_X1 - STUB_W  # 38.50
STUB_LEG_X1 = STUB_X1  # 39.50
STUB_BBOX_H = STUB_V_EXT  # 4.05  alto del bounding box del stub en L

# Borde derecho de la antena: solo tiene cobre desde y=0 hasta el fin del brazo
# central -> esa es la cota 10.50 del plano.
RIGHT_EDGE_H = ARM_TOP_H + SLOT_W + ARM_MID_H  # 10.50

# Bordes de la PCB de referencia en coordenadas locales del footprint
BOARD_LEFT = -SIDE_MARGIN  # -5.25
BOARD_RIGHT = ANT_W + SIDE_MARGIN  # 44.75
BOARD_TOP = -EDGE_TO_ANT_TOP  # -4.00

# Posicion del pad de C101 (lado antena) sobre la linea CPWG
C101_PAD_Y = Y_GND + CPWG_LEN  # 21.63

# Ancla del pad de feed (pad 1 del footprint).
# Se prolonga FEED_ANCHOR_DOWN por debajo del borde del plano para que la pista
# CPWG pueda arrancar FUERA del area de keepout y aun caer dentro del pad: una
# pista de 1.00 mm tiene casquete redondo de 0.50 mm de radio en su extremo, asi
# que su punto de arranque tiene que quedar a >= 0.50 mm del borde del keepout.
# Ese cobre extra cae justo sobre el trazado de la linea de feed, que en el plano
# sigue recta hasta C101, asi que no cambia la geometria de la antena.
FEED_ANCHOR_DOWN = 1.50
FEED_ANCHOR_UP = 0.50
FEED_ANCHOR_H = FEED_ANCHOR_UP + FEED_ANCHOR_DOWN  # 2.00
FEED_ANCHOR_CY = Y_GND + (FEED_ANCHOR_DOWN - FEED_ANCHOR_UP) / 2.0  # 16.53
FEED_ANCHOR_EXTRA_AREA = FEED_W * FEED_ANCHOR_DOWN  # 1.50 mm^2 por debajo del plano


# --------------------------------------------------------------------------
# Cobre de la antena
# --------------------------------------------------------------------------

def _rect(x0: float, y0: float, x1: float, y1: float):
    """Rectangulo como (x0, y0, x1, y1) normalizado."""
    return (min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1))


# Rectangulos NOMINALES: describen la geometria exacta del plano, sin solaparse.
# Se usan para calcular area, para el render SVG y para las comprobaciones.
COPPER_RECTS_NOMINAL = [
    ("brazo_superior", _rect(0.0, Y_TOP, ANT_W, Y_SLOT1_T)),
    ("puente_derecho", _rect(SLOT1_LEN, Y_SLOT1_T, ANT_W, Y_SLOT1_B)),
    ("brazo_central", _rect(0.0, Y_SLOT1_B, ANT_W, Y_SLOT2_T)),
    ("puente_izquierdo", _rect(0.0, Y_SLOT2_T, BRIDGE_L, Y_SLOT2_B)),
    ("brazo_inferior", _rect(0.0, Y_SLOT2_B, ARM_BOT_X1, Y_BOT)),
    ("stub_L_horizontal", _rect(STUB_X0, STUB_TOP_Y, STUB_X1, Y_BOT)),
    ("stub_L_vertical", _rect(STUB_LEG_X0, Y_BOT, STUB_LEG_X1, Y_GND)),
    ("linea_feed", _rect(FEED_X0, Y_BOT, FEED_X1, Y_GND)),
]

# Rectangulos SOLAPADOS: misma union que los nominales, pero cada pieza invade
# 0.50 mm la pieza vecina. KiCad une las primitivas de un mismo pad, y el solape
# garantiza que no quede ninguna rendija de 0 mm en el Gerber. El solape siempre
# cae DENTRO de un rectangulo nominal vecino, asi que la union no cambia.
_OV = 0.50

COPPER_RECTS_MERGED = [
    ("brazo_superior", _rect(0.0, Y_TOP, ANT_W, Y_SLOT1_T)),
    ("puente_derecho", _rect(SLOT1_LEN, Y_SLOT1_T - _OV, ANT_W, Y_SLOT1_B + _OV)),
    ("brazo_central", _rect(0.0, Y_SLOT1_B, ANT_W, Y_SLOT2_T)),
    ("puente_izquierdo", _rect(0.0, Y_SLOT2_T - _OV, BRIDGE_L, Y_SLOT2_B + _OV)),
    ("brazo_inferior", _rect(0.0, Y_SLOT2_B, ARM_BOT_X1, Y_BOT)),
    ("stub_L_horizontal", _rect(STUB_X0 - _OV, STUB_TOP_Y, STUB_X1, Y_BOT)),
    ("stub_L_vertical", _rect(STUB_LEG_X0, Y_BOT - _OV, STUB_LEG_X1, Y_GND)),
    ("linea_feed", _rect(FEED_X0, Y_BOT - _OV, FEED_X1, Y_GND)),
]

# Contorno cerrado del cobre, recorrido en sentido horario desde el origen.
# La antena es una figura SIMPLEMENTE CONEXA (ambas ranuras estan abiertas por un
# borde), asi que un unico poligono sin huecos la describe por completo.
#
# El area de este poligono se compara con la suma de COPPER_RECTS_NOMINAL en
# check(): si el recorrido se equivoca en un vertice, las dos areas dejan de
# coincidir y la comprobacion falla.
OUTLINE = [
    (0.0, Y_TOP),                      # 1  esquina superior izquierda
    (ANT_W, Y_TOP),                    # 2  borde superior, hacia la derecha
    (ANT_W, Y_SLOT2_T),                # 3  borde derecho: brazo sup + puente der + brazo central
    (BRIDGE_L, Y_SLOT2_T),             # 4  borde inferior del brazo central (ranura 2)
    (BRIDGE_L, Y_SLOT2_B),             # 5  extremo cerrado de la ranura 2 = puente izquierdo
    (ARM_BOT_X1, Y_SLOT2_B),           # 6  borde superior del brazo inferior
    (ARM_BOT_X1, STUB_TOP_Y),          # 7  baja hasta el arranque del stub en L
    (STUB_X1, STUB_TOP_Y),             # 8  borde superior del tramo horizontal del stub
    (STUB_LEG_X1, Y_GND),              # 9  borde derecho del stub, baja hasta el plano
    (STUB_LEG_X0, Y_GND),              # 10 extremo inferior de la pata del stub
    (STUB_LEG_X0, Y_BOT),              # 11 sube por el lado interno de la pata
    (ARM_BOT_X1, Y_BOT),               # 12 borde inferior del tramo horizontal del stub
    (ARM_BOT_X1, Y_GND),               # 13 borde derecho de la linea de feed, baja al plano
    (FEED_X0, Y_GND),                  # 14 extremo inferior de la linea de feed
    (FEED_X0, Y_BOT),                  # 15 sube por el lado izquierdo del feed
    (0.0, Y_BOT),                      # 16 borde inferior del brazo inferior
    (0.0, Y_SLOT1_B),                  # 17 borde izquierdo: brazo inf + puente izq + brazo central
    (SLOT1_LEN, Y_SLOT1_B),            # 18 borde inferior de la ranura 1
    (SLOT1_LEN, Y_SLOT1_T),            # 19 extremo cerrado de la ranura 1 = puente derecho
    (0.0, Y_SLOT1_T),                  # 20 borde superior de la ranura 1
]

# Ranuras de la antena (para el render; NO son cobre)
SLOTS = [
    ("ranura_1_abierta_izquierda", _rect(0.0, Y_SLOT1_T, SLOT1_LEN, Y_SLOT1_B)),
    ("ranura_2_abierta_derecha", _rect(BRIDGE_L, Y_SLOT2_T, ANT_W, Y_SLOT2_B)),
]

# --------------------------------------------------------------------------
# Area de keepout / courtyard
# --------------------------------------------------------------------------
# Toda la region por encima del plano de tierra queda libre de cobre, planos,
# vias y componentes, en AMBAS capas. Es el requisito RF de una IFA: no puede
# haber plano por debajo ni alrededor del elemento radiante.

KEEPOUT = _rect(BOARD_LEFT, BOARD_TOP, BOARD_RIGHT, Y_GND)


# --------------------------------------------------------------------------
# Fila de ranuras de TROQUELADO (solo EVM - ver docs/geometria-antena.md)
# --------------------------------------------------------------------------
# Las 5 ranuras redondeadas que el plano dibuja en magenta entre la antena y el
# plano de tierra NO son un detalle de RF: son la linea por la que se ARRANCA la
# antena. La seccion 1.9 del User Manual ("EVB Radiation -> Conduction Change")
# lo deja explicito: (1) PCB ANT remove, (2) C101 remove, (3) CON101 RF SMA
# connector insertion. El esquematico marca esa misma zona con un recuadro "CUT"
# que encierra ANT1 y C101.
#
# Consecuencia de diseno: en el PRODUCTO no deben ir. Una antena pensada para
# romperse a mano es un punto de fallo mecanico en una cerradura.
#
# Cotas MEDIDAS sobre el bitmap original del User Manual Rev 1.4 (522 x 417 px,
# escala 8.203 px/mm -> resolucion 0.12 mm/px). Ya no son valores a ojo, pero
# siguen sin estar cotadas en el plano: la incertidumbre es +/-0.12 mm.

SLOTROW_Y0 = 14.08  # borde superior, 1.08 mm por debajo del cobre de la antena
SLOTROW_Y1 = 15.85  # borde inferior, ~0.2 mm por encima del borde del plano
SLOTROW_H = SLOTROW_Y1 - SLOTROW_Y0  # 1.77

SLOTROW_X = [
    ("A", -2.50, 7.31),
    ("B", 10.06, 20.36),
    ("C", 23.16, 33.16),   # termina 0.34 mm antes del feed  (x 33.50)
    ("D", 34.93, 38.22),   # entre feed (34.50) y pata del stub (38.50)
    ("E", 39.93, 42.49),   # arranca 0.43 mm despues de la pata del stub
]


# --------------------------------------------------------------------------
# Comprobaciones de consistencia de la cadena de cotas
# --------------------------------------------------------------------------

def copper_area() -> float:
    """Area de cobre en mm^2, a partir de los rectangulos nominales (sin solape)."""
    return sum((x1 - x0) * (y1 - y0) for _, (x0, y0, x1, y1) in COPPER_RECTS_NOMINAL)


def outline_area() -> float:
    """Area encerrada por OUTLINE (formula del zapatero). Debe igualar copper_area()."""
    n = len(OUTLINE)
    s = sum(
        OUTLINE[i][0] * OUTLINE[(i + 1) % n][1] - OUTLINE[(i + 1) % n][0] * OUTLINE[i][1]
        for i in range(n)
    )
    return abs(s) / 2.0


def outline_is_rectilinear() -> bool:
    """Todo segmento de OUTLINE debe ser puramente horizontal o vertical."""
    n = len(OUTLINE)
    for i in range(n):
        (x0, y0), (x1, y1) = OUTLINE[i], OUTLINE[(i + 1) % n]
        if abs(x0 - x1) > 1e-9 and abs(y0 - y1) > 1e-9:
            return False
        if abs(x0 - x1) < 1e-9 and abs(y0 - y1) < 1e-9:
            return False  # vertice duplicado
    return True


def current_path_length() -> float:
    """
    Longitud del recorrido de corriente en forma de "2", medida por las lineas
    medias de los brazos y los puentes. Es una estimacion de primer orden: en
    brazos de 5 mm la corriente no sigue la linea media, asi que este numero
    sirve para comparar variantes, no para predecir la resonancia.
    """
    y_top_c = ARM_TOP_H / 2.0                       # 2.50
    y_mid_c = Y_SLOT1_B + ARM_MID_H / 2.0           # 8.00
    y_bot_c = Y_SLOT2_B + ARM_BOT_H / 2.0           # 12.00
    x_bridge_r = SLOT1_LEN + BRIDGE_R / 2.0         # 36.75
    x_bridge_l = BRIDGE_L / 2.0                     # 3.00

    return (
        (Y_BOT - y_bot_c)                       # feed -> linea media del brazo inferior
        + (FEED_CENTER_X - x_bridge_l)          # brazo inferior hacia la izquierda
        + (y_bot_c - y_mid_c)                   # puente izquierdo
        + (x_bridge_r - x_bridge_l)             # brazo central hacia la derecha
        + (y_mid_c - y_top_c)                   # puente derecho
        + (x_bridge_r - 0.0)                    # brazo superior hasta el extremo abierto
    )


def check() -> list[str]:
    """Devuelve la lista de comprobaciones de la cadena de cotas."""
    out = []

    def ok(label, got, want, tol=1e-9):
        status = "OK " if abs(got - want) <= tol else "FALLA"
        out.append(f"[{status}] {label}: {got:.3f} (esperado {want:.3f})")

    ok("ancho antena = 39.50", ANT_W, 39.50)
    ok("alto antena = 5.00+0.50+5.00+0.50+2.00", ANT_H, 13.00)
    ok("cadena vertical 4.00 + 13.00 + 3.03 = 20.03",
       EDGE_TO_ANT_TOP + ANT_H + ANT_TO_GND, EDGE_TO_GND)
    ok("puente derecho = 39.50 - 34.00", BRIDGE_R, 5.50)
    ok("largo ranura 2 = 39.50 - 6.00", SLOT2_LEN, 33.50)
    ok("alto borde derecho (cota 10.50)", RIGHT_EDGE_H, 10.50)
    ok("antena centrada: margen lateral", SIDE_MARGIN, 5.25)
    ok("stub en L: tramo horizontal = 5.00", STUB_X1 - STUB_X0, 5.00)
    ok("stub en L: borde derecho coincide con el de la antena", STUB_X1, ANT_W)
    # Exacta, no con tolerancia: la cota 4.05 del plano manda sobre la geometria.
    ok("stub en L: extension vertical = cota 4.05 del plano", Y_GND - STUB_TOP_Y, 4.05)
    ok("stub en L: alto del tramo horizontal = 4.05 - 3.03", STUB_RUN_H, 1.02)
    ok("stub en L: ancho de la pata = 1.00", STUB_LEG_X1 - STUB_LEG_X0, 1.00)
    ok("feed centrado en el fin de la ranura 1", FEED_CENTER_X, 34.00)
    ok("feed y stub contiguos (topologia IFA)", STUB_X0 - FEED_X1, 0.0)
    ok("pata del stub llega al plano de tierra", Y_GND, EDGE_TO_GND - EDGE_TO_ANT_TOP)
    ok("area de cobre", copper_area(), 480.91, tol=1e-3)
    ok("area del contorno == suma de rectangulos", outline_area(), copper_area(), tol=1e-6)
    out.append(
        f"[{'OK ' if outline_is_rectilinear() else 'FALLA'}] "
        f"contorno rectilineo (solo tramos H/V, sin vertices repetidos)"
    )

    # La union de los rectangulos solapados debe dar la misma area que la nominal
    from itertools import product
    xs = sorted({v for _, r in COPPER_RECTS_MERGED for v in (r[0], r[2])}
                | {v for _, r in COPPER_RECTS_NOMINAL for v in (r[0], r[2])})
    ys = sorted({v for _, r in COPPER_RECTS_MERGED for v in (r[1], r[3])}
                | {v for _, r in COPPER_RECTS_NOMINAL for v in (r[1], r[3])})

    def union_area(rects):
        total = 0.0
        for (x0, x1), (y0, y1) in product(zip(xs, xs[1:]), zip(ys, ys[1:])):
            cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
            for _, (rx0, ry0, rx1, ry1) in rects:
                if rx0 <= cx <= rx1 and ry0 <= cy <= ry1:
                    total += (x1 - x0) * (y1 - y0)
                    break
        return total

    a_nom, a_mrg = union_area(COPPER_RECTS_NOMINAL), union_area(COPPER_RECTS_MERGED)
    ok("union solapada == union nominal", a_mrg, a_nom, tol=1e-6)

    return out


if __name__ == "__main__":
    for line in check():
        print(line)
    print()
    print(f"Area de cobre        : {copper_area():.2f} mm^2")
    print(f"Recorrido de corriente: {current_path_length():.2f} mm (estimacion linea media)")
    print(f"Keepout              : {KEEPOUT}")
