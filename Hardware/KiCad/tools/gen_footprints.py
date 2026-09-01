#!/usr/bin/env python3
"""
Genera los footprints KiCad de la antena a partir de antenna_geometry.py.

    python3 gen_footprints.py [carpeta]

Sin argumento escribe en antenna_geometry.LIB_DIR, que es la carpeta donde vive
el LSM110A: `Hardware/v0-replica-sji/kicad-lib/`.

Salida (los cuatro en la MISMA carpeta, bajo el nickname 0G-LockControl):
    ANT_IFA_915MHz_LSM110A.kicad_mod               antena completa (cobre + keepout + doc)
    ANT_LSM110A_BreakAwaySlots_EVM_ONLY.kicad_mod  troquelado del EVM (NO en el producto)
    C_0402_1005Metric_0G.kicad_mod                 0402 de la red de matching
    TP_Coax_50R_NanoVNA.kicad_mod                  punto de test coaxial 50 ohm

Formato: KiCad 7 (version 20221018), compatible con KiCad 7/8/9.
La sintaxis exacta se verifico contra la salida de pcbnew.IO_MGR (ver docs/verificacion.md).
Los UUID son deterministas, asi que regenerar no produce diff espurio en git.
"""

from __future__ import annotations

import sys
import uuid
from pathlib import Path

import antenna_geometry as G

NS = uuid.UUID("6f0a1c2e-9b3d-4f5a-8c7b-0d1e2f3a4b5c")  # namespace propio del proyecto


def uid(*parts) -> str:
    return str(uuid.uuid5(NS, ":".join(str(p) for p in parts)))


def n(value: float) -> str:
    """Numero en el formato de KiCad: hasta 6 decimales, sin ceros de relleno."""
    s = f"{value:.6f}".rstrip("0").rstrip(".")
    return "0" if s in ("", "-0") else s


def xy(x: float, y: float) -> str:
    return f"{n(x)} {n(y)}"


# ---------------------------------------------------------------- primitivas

def fp_line(tag, i, x0, y0, x1, y1, layer, width, stroke="default"):
    return (
        f'  (fp_line (start {xy(x0, y0)}) (end {xy(x1, y1)})\n'
        f'    (stroke (width {n(width)}) (type {stroke})) (layer "{layer}") '
        f'(tstamp {uid(tag, "line", i)}))'
    )


def fp_rect(tag, i, x0, y0, x1, y1, layer, width, fill="none", stroke="default"):
    return (
        f'  (fp_rect (start {xy(x0, y0)}) (end {xy(x1, y1)})\n'
        f'    (stroke (width {n(width)}) (type {stroke})) (fill {fill}) (layer "{layer}") '
        f'(tstamp {uid(tag, "rect", i)}))'
    )


def fp_arc(tag, i, sx, sy, mx, my, ex, ey, layer, width):
    return (
        f'  (fp_arc (start {xy(sx, sy)}) (mid {xy(mx, my)}) (end {xy(ex, ey)})\n'
        f'    (stroke (width {n(width)}) (type default)) (layer "{layer}") '
        f'(tstamp {uid(tag, "arc", i)}))'
    )


def fp_poly(tag, i, pts, layer, width, fill="none"):
    body = "\n".join(f"      (xy {xy(x, y)})" for x, y in pts)
    return (
        f'  (fp_poly\n    (pts\n{body}\n    )\n'
        f'    (stroke (width {n(width)}) (type default)) (fill {fill}) (layer "{layer}") '
        f'(tstamp {uid(tag, "poly", i)}))'
    )


def fp_text(tag, kind, text, x, y, layer, size=1.0, thickness=0.15, extra=""):
    return (
        f'  (fp_text {kind} "{text}" (at {xy(x, y)}) (layer "{layer}"){extra}\n'
        f'    (effects (font (size {n(size)} {n(size)}) (thickness {n(thickness)})))\n'
        f'    (tstamp {uid(tag, "text", kind, text)})\n  )'
    )


def rule_area(tag, name, rect, layers="F&B.Cu"):
    x0, y0, x1, y1 = rect
    pts = "\n".join(
        f"        (xy {xy(px, py)})"
        for px, py in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    )
    return (
        f'  (zone (net 0) (net_name "") (layers "{layers}") (tstamp {uid(tag, "zone", name)}) '
        f'(name "{name}") (hatch edge 0.5)\n'
        f'    (connect_pads (clearance 0))\n'
        f'    (min_thickness 0.25) (filled_areas_thickness no)\n'
        f'    (keepout (tracks not_allowed) (vias not_allowed) (pads allowed) '
        f'(copperpour not_allowed) (footprints allowed))\n'
        f'    (fill (thermal_gap 0.5) (thermal_bridge_width 0.5))\n'
        f'    (polygon\n      (pts\n{pts}\n      )\n    )\n  )'
    )


# ---------------------------------------------------------------- antena

FP_NAME = "ANT_IFA_915MHz_LSM110A"

DESCR = (
    "Antena IFA ranurada integrada en PCB para LSM110A. Banda 902-928 MHz (Sigfox RC2/RC4): el S11 medido por SJI da -16.4 dB a 902.1 MHz, -17.6 dB a 921.6 MHz y -29.3 dB a 928 MHz. NO sirve para EU868 sin reajustar: a 868.1 MHz el S11 de referencia es solo -8.7 dB (VSWR 2.17). "
    "Redibujo parametrico del plano '1.5 Antenna Dimension': placa 39.50 x 13.00 mm con dos "
    "ranuras de 0.50 mm que fuerzan un recorrido de corriente en forma de '2', stub en L de "
    "cortocircuito a GND y linea de alimentacion de 1.00 mm. Todo el cobre es un solo poligono. "
    "Origen del footprint = esquina superior izquierda del cobre. Requiere el borde superior de "
    "la PCB a 4.00 mm por encima del origen y el plano de tierra empezando exactamente a "
    "16.03 mm por debajo. Footprint tipo net-tie: el pad 1 (FEED) y el pad 2 (GND) estan unidos "
    "a proposito por el stub, como en cualquier IFA."
)

TAGS = "antenna IFA PCB 915MHz 902-928 sub-GHz Sigfox RC2 RC4 LSM110A net-tie"


def build_antenna() -> str:
    tag = FP_NAME
    L: list[str] = []

    L.append(f'(footprint "{FP_NAME}" (version 20221018) (generator 0g_antenna_gen)')
    L.append('  (layer "F.Cu")')
    L.append(f'  (descr "{DESCR}")')
    L.append(f'  (tags "{TAGS}")')
    L.append('  (attr smd exclude_from_pos_files exclude_from_bom allow_soldermask_bridges)')
    L.append('  (net_tie_pad_groups "1, 2")')

    # ---- textos ------------------------------------------------------------
    cx = G.ANT_W / 2.0
    L.append(fp_text(tag, "reference", "REF**", cx, -2.80, "F.SilkS", 0.8, 0.12))
    L.append(fp_text(tag, "value", FP_NAME, cx, 17.80, "F.Fab", 1.0, 0.15))
    L.append(fp_text(tag, "user", "${REFERENCE}", cx, -1.30, "F.Fab", 0.8, 0.12))

    # ---- documentacion en F.Fab: contorno exacto del cobre -----------------
    L.append(fp_poly(tag, "cobre", G.OUTLINE, "F.Fab", 0.05))

    # ---- courtyard: cubre todo el keepout, asi ningun componente cae dentro -
    L.append(fp_rect(tag, "crtyd", *G.KEEPOUT, "F.CrtYd", 0.05))

    # ---- referencias mecanicas en Dwgs.User --------------------------------
    kx0, ky0, kx1, ky1 = G.KEEPOUT
    # borde superior de la PCB
    L.append(fp_line(tag, "edge_top", kx0, G.BOARD_TOP, kx1, G.BOARD_TOP,
                     "Dwgs.User", 0.12, "dash"))
    # bordes laterales de la PCB de referencia de 50 mm
    L.append(fp_line(tag, "edge_l", kx0, G.BOARD_TOP, kx0, G.Y_GND, "Dwgs.User", 0.12, "dash"))
    L.append(fp_line(tag, "edge_r", kx1, G.BOARD_TOP, kx1, G.Y_GND, "Dwgs.User", 0.12, "dash"))
    # linea donde DEBE arrancar el plano de tierra
    L.append(fp_line(tag, "gnd_edge", kx0, G.Y_GND, kx1, G.Y_GND, "Dwgs.User", 0.15))

    L.append(fp_text(tag, "user", "borde superior PCB (4.00 sobre el origen)",
                     cx, G.BOARD_TOP - 0.55, "Dwgs.User", 0.6, 0.1))
    L.append(fp_text(tag, "user", "el plano GND arranca AQUI (y = 16.03)",
                     cx - 8.0, G.Y_GND + 0.75, "Dwgs.User", 0.6, 0.1))
    L.append(fp_text(tag, "user", "keepout: sin plano / sin vias / sin pistas / sin componentes",
                     cx, G.Y_TOP - 2.10, "Dwgs.User", 0.6, 0.1))

    # ---- cotas dibujadas, sobre Cmts.User ----------------------------------
    # Un .kicad_mod no admite objetos de cota de KiCad (esos son de placa), asi
    # que se dibujan con lineas: dos marcas testigo y el tramo entre ellas. El
    # objetivo es que las cotas del plano se puedan COMPROBAR sobre el footprint
    # en vez de tener que creerselas: basta poner la regla entre las dos marcas.
    #
    # OJO al medir en KiCad: la regla se engancha a la rejilla. Con rejilla de
    # 0.5 o 1 mm, 3.03 se lee como 3.000 y 4.05 como 4.000. Hay que poner la
    # rejilla en 0.01 mm, o mantener pulsada Ctrl para desactivar el enganche.
    def dim_v(key, x, y0, y1, label, from_x, lx):
        """
        Cota vertical con LINEAS DE EXTENSION, como las dibuja el plano de SJI:
        salen del canto de cobre que se mide y llegan hasta pasada la linea de
        cota. Sin ellas la cota queda flotando en el vacio y no se ve que mide
        -- que es justo lo que le pasaba a la primera version.

            from_x  x del canto de cobre del que sale la cota
            x       x de la linea de cota
        """
        s = 1.0 if x > from_x else -1.0
        gap = 0.15 * s      # hueco entre el cobre y el arranque de la extension
        over = 0.40 * s     # la extension sobrepasa la linea de cota
        w, arr = 0.05, 0.45  # grosor y largo de las puntas de flecha
        out = []
        for n, y in (("e0", y0), ("e1", y1)):
            out.append(fp_line(tag, f"{key}_{n}", from_x + gap, y, x + over, y,
                               "Cmts.User", w))
        out.append(fp_line(tag, f"{key}_ln", x, y0, x, y1, "Cmts.User", w))
        # puntas de flecha: dos trazos oblicuos hacia dentro en cada extremo
        for n, y, d in (("a0", y0, +1.0), ("a1", y1, -1.0)):
            for k, dx in ((0, -0.20), (1, +0.20)):
                out.append(fp_line(tag, f"{key}_{n}{k}", x, y, x + dx, y + d * arr,
                                   "Cmts.User", w))
        out.append(fp_text(tag, "user", label, lx, (y0 + y1) / 2.0,
                           "Cmts.User", 0.5, 0.08))
        return out

    # Solo se acotan cantos que EXISTEN en este footprint.
    # 3.03: fondo del cobre -> borde del plano. Sale del canto izquierdo (x=0).
    L += dim_v("dim303", -2.30, G.Y_BOT, G.Y_GND, "3.03", from_x=0.0, lx=-3.30)
    # 4.05: techo del tramo del stub -> borde del plano. Sale del canto derecho.
    L += dim_v("dim405", 41.60, G.STUB_TOP_Y, G.Y_GND, "4.05",
               from_x=G.ANT_W, lx=42.60)
    #
    # La cota 5.60 del plano NO se dibuja aqui, a proposito. Va del borde del
    # plano al pad de C101/L101, y ese condensador es un componente de PLACA:
    # dentro del footprint no hay cobre a esa altura, asi que la cota apuntaria
    # al vacio. Una cota que no acota nada estorba y hace dudar de la geometria.
    # Vive donde el cobre existe: como objeto de cota de KiCad en la placa de
    # prueba (ver gen_test_board.py) y comprobada en verify_board.py.

    # En la banda vacia entre el cobre y el plano, a la altura del feed: asi
    # nada del footprint cuelga por debajo del cobre.
    L.append(fp_text(tag, "user", "FEED 1.00 -> CPWG 1.00 / 0.15",
                     G.FEED_CENTER_X - 12.0, G.Y_GND - 1.20, "Cmts.User", 0.6, 0.1))
    # Corto y fuera de la linea de cota de 4.05, que pasa por x = 41.60.
    L.append(fp_text(tag, "user",
                     f"stub {G.STUB_RUN:.2f} x {G.STUB_RUN_H:.2f}",
                     36.0, G.Y_GND - 1.20, "Cmts.User", 0.5, 0.08))

    # ---- pad 1: TODO el cobre de la antena ---------------------------------
    # El ancla del pad se prolonga 1.50 mm por debajo del borde del plano para
    # que la pista CPWG pueda arrancar fuera del keepout (ver antenna_geometry).
    px, py = G.FEED_CENTER_X, G.FEED_ANCHOR_CY
    prims = []
    for i, (name, (x0, y0, x1, y1)) in enumerate(G.COPPER_RECTS_MERGED):
        pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
        body = "\n".join(f"          (xy {xy(x - px, y - py)})" for x, y in pts)
        prims.append(f"      (gr_poly\n        (pts\n{body}\n        )\n        (width 0) (fill yes))")
    prims_txt = "\n".join(prims)

    L.append(
        f'  (pad "1" smd custom (at {xy(px, py)}) '
        f'(size {n(G.FEED_W)} {n(G.FEED_ANCHOR_H)}) '
        f'(layers "F.Cu")\n'
        f'    (zone_connect 0)\n'
        f'    (options (clearance outline) (anchor rect))\n'
        f'    (primitives\n{prims_txt}\n'
        f'    ) (tstamp {uid(tag, "pad", 1)}))'
    )

    # ---- pad 2: cortocircuito del stub al plano de tierra ------------------
    gx = (G.STUB_LEG_X0 + G.STUB_LEG_X1) / 2.0
    L.append(
        f'  (pad "2" smd rect (at {xy(gx, G.Y_GND)}) '
        f'(size {n(G.STUB_W)} {n(G.STUB_W)}) (layers "F.Cu")\n'
        f'    (zone_connect 2) (tstamp {uid(tag, "pad", 2)}))'
    )

    # ---- rule area (keepout) en ambas capas de cobre -----------------------
    L.append(rule_area(tag, "ANT_KEEPOUT", G.KEEPOUT))

    L.append(")")
    return "\n".join(L) + "\n"


# ---------------------------------------------------------------- slot row

SLOT_FP_NAME = "ANT_LSM110A_BreakAwaySlots_EVM_ONLY"

SLOT_DESCR = (
    "SOLO EVM - NO PONER EN EL PRODUCTO. Fila de 5 ranuras redondeadas entre la antena y "
    "el plano de tierra. No es un detalle de RF: es la linea por la que se ARRANCA la antena. "
    "La seccion 1.9 del User Manual LSM110A ('EVB Radiation -> Conduction Change') lo detalla: "
    "(1) PCB ANT remove, (2) C101 remove, (3) CON101 RF SMA connector insertion; y el "
    "esquematico marca esa zona con un recuadro 'CUT' que encierra ANT1 y C101. Sirve para "
    "pasar la placa de evaluacion de medida radiada a medida conducida. En una cerradura, una "
    "antena que se puede romper a mano es un punto de fallo mecanico. Cotas medidas sobre el "
    "bitmap original del manual (8.203 px/mm), incertidumbre +/-0.12 mm: el plano NO las cota. "
    "Se coloca con el MISMO origen que ANT_IFA_915MHz_LSM110A."
)


def build_slotrow() -> str:
    tag = SLOT_FP_NAME
    L: list[str] = []
    L.append(f'(footprint "{SLOT_FP_NAME}" (version 20221018) (generator 0g_antenna_gen)')
    L.append('  (layer "F.Cu")')
    L.append(f'  (descr "{SLOT_DESCR}")')
    L.append('  (tags "PCB slot break-away cutout antenna LSM110A EVM-only")')
    L.append('  (attr exclude_from_pos_files exclude_from_bom)')

    cx = G.ANT_W / 2.0
    L.append(fp_text(tag, "reference", "REF**", cx, G.SLOTROW_Y0 - 1.2, "F.SilkS", 0.8, 0.12))
    L.append(fp_text(tag, "value", SLOT_FP_NAME, cx, G.SLOTROW_Y1 + 1.2, "F.Fab", 0.7, 0.1))

    y0, y1 = G.SLOTROW_Y0, G.SLOTROW_Y1
    r = G.SLOTROW_H / 2.0
    yc = (y0 + y1) / 2.0

    for name, x0, x1 in G.SLOTROW_X:
        # tramos rectos
        L.append(fp_line(tag, f"{name}_top", x0 + r, y0, x1 - r, y0, "Edge.Cuts", 0.05))
        L.append(fp_line(tag, f"{name}_bot", x0 + r, y1, x1 - r, y1, "Edge.Cuts", 0.05))
        # semicirculos en los extremos
        L.append(fp_arc(tag, f"{name}_left", x0 + r, y1, x0, yc, x0 + r, y0, "Edge.Cuts", 0.05))
        L.append(fp_arc(tag, f"{name}_right", x1 - r, y0, x1, yc, x1 - r, y1, "Edge.Cuts", 0.05))
        # copia en F.Fab para que se vea en el editor sin activar Edge.Cuts
        L.append(fp_rect(tag, f"{name}_fab", x0, y0, x1, y1, "F.Fab", 0.05))

    L.append(fp_text(tag, "user", "SOLO EVM: linea de troquelado, no poner en el producto",
                     cx, y1 + 2.4, "Cmts.User", 0.6, 0.1))
    L.append(")")
    return "\n".join(L) + "\n"


# ---------------------------------------------------------------- apoyo RF
# Dos footprints que necesita la placa de prueba. Se definen aqui para que el
# proyecto no dependa de las bibliotecas estandar de KiCad.

CAP_NAME = "C_0402_1005Metric_0G"
CAP_PAD = 0.60  # pad cuadrado -> el footprint se puede girar sin recalcular nada
CAP_PITCH = 0.485  # centro de pad respecto al centro del componente


def build_cap0402() -> str:
    tag = CAP_NAME
    L = [
        f'(footprint "{CAP_NAME}" (version 20221018) (generator 0g_antenna_gen)',
        '  (layer "F.Cu")',
        '  (descr "Condensador 0402 (1005 metrico), land IPC nominal: 2 pads de '
        '0.60 x 0.60 mm a +/-0.485 mm. Definido en el proyecto para no depender de '
        'las bibliotecas estandar de KiCad. Uso previsto: C101, condensador serie de '
        'acoplo/matching en la linea RF de la antena.")',
        '  (tags "capacitor 0402 1005 RF matching")',
        '  (attr smd)',
        fp_text(tag, "reference", "REF**", 0, -1.10, "F.SilkS", 0.8, 0.12),
        fp_text(tag, "value", CAP_NAME, 0, 1.10, "F.Fab", 0.6, 0.1),
        fp_text(tag, "user", "${REFERENCE}", 0, 0, "F.Fab", 0.4, 0.06),
        # contorno del cuerpo 1.00 x 0.50
        fp_rect(tag, "body", -0.50, -0.25, 0.50, 0.25, "F.Fab", 0.05),
        # marcas de serigrafia fuera de los pads
        fp_line(tag, "silk_t", -0.16, -0.40, 0.16, -0.40, "F.SilkS", 0.12),
        fp_line(tag, "silk_b", -0.16, 0.40, 0.16, 0.40, "F.SilkS", 0.12),
        fp_rect(tag, "crtyd", -0.91, -0.51, 0.91, 0.51, "F.CrtYd", 0.05),
    ]
    for i, sx in ((1, -1), (2, +1)):
        L.append(
            f'  (pad "{i}" smd roundrect (at {xy(sx * CAP_PITCH, 0)}) '
            f'(size {n(CAP_PAD)} {n(CAP_PAD)}) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25) '
            f'(tstamp {uid(tag, "pad", i)}))'
        )
    L.append(")")
    return "\n".join(L) + "\n"


COAX_NAME = "TP_Coax_50R_NanoVNA"
COAX_SIG_W, COAX_SIG_H = 1.00, 1.60
COAX_GND_W, COAX_GND_H = 1.60, 1.60


def build_coax() -> str:
    """Isla de 3 pads para soldar un pigtail coaxial (medida con NanoVNA)."""
    tag = COAX_NAME
    gnd_dx = COAX_SIG_W / 2.0 + G.CPWG_GAP + COAX_GND_W / 2.0  # 0.5 + 0.15 + 0.8 = 1.45
    L = [
        f'(footprint "{COAX_NAME}" (version 20221018) (generator 0g_antenna_gen)',
        '  (layer "F.Cu")',
        '  (descr "Isla de 3 pads para soldar un pigtail coaxial de 50 ohm (RG-178 o '
        'semirrigido) y medir S11 con NanoVNA. Pad 1 = malla/GND izquierda, pad 2 = vivo, '
        'pad 3 = malla/GND derecha. La separacion vivo-malla es 0.15 mm, la misma del CPWG, '
        'para no crear discontinuidad. Solo para la placa de prueba y ajuste: no va en el '
        'producto.")',
        '  (tags "coax test point NanoVNA S11 RF 50ohm")',
        '  (attr smd exclude_from_pos_files exclude_from_bom)',
        fp_text(tag, "reference", "REF**", 0, -1.90, "F.SilkS", 0.8, 0.12),
        fp_text(tag, "value", COAX_NAME, 0, 1.90, "F.Fab", 0.6, 0.1),
        fp_rect(tag, "crtyd", -(gnd_dx + COAX_GND_W / 2.0) - 0.25, -COAX_GND_H / 2.0 - 0.25,
                (gnd_dx + COAX_GND_W / 2.0) + 0.25, COAX_GND_H / 2.0 + 0.25, "F.CrtYd", 0.05),
    ]
    pads = [
        ("1", -gnd_dx, COAX_GND_W, COAX_GND_H),
        ("2", 0.0, COAX_SIG_W, COAX_SIG_H),
        ("3", +gnd_dx, COAX_GND_W, COAX_GND_H),
    ]
    for num, px, w, h in pads:
        L.append(
            f'  (pad "{num}" smd rect (at {xy(px, 0)}) (size {n(w)} {n(h)}) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (tstamp {uid(tag, "pad", num)}))'
        )
    L.append(")")
    return "\n".join(L) + "\n"


def main() -> None:
    # Los cuatro footprints van a la MISMA carpeta, junto al LSM110A: un solo
    # path registrado en KiCad y un solo nickname para todo el proyecto.
    lib = Path(sys.argv[1]) if len(sys.argv) > 1 else G.lib_dir(__file__)
    lib.mkdir(parents=True, exist_ok=True)

    for name, text in (
        (FP_NAME, build_antenna()),
        (SLOT_FP_NAME, build_slotrow()),
        (CAP_NAME, build_cap0402()),
        (COAX_NAME, build_coax()),
    ):
        p = lib / f"{name}.kicad_mod"
        p.write_text(text, encoding="utf-8")
        print(f"escrito: {p}  ({len(text)} bytes)")


if __name__ == "__main__":
    main()
