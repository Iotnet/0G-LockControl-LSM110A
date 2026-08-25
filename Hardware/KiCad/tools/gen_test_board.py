#!/usr/bin/env python3
"""
Genera la placa de prueba y ajuste de la antena, usando la API de pcbnew.

    python3 gen_test_board.py ../antenna-test-board

Que produce:
  - antenna-test-board.kicad_pcb   placa 50 x 80 mm, 2 capas, con la antena colocada
                                   segun el plano, plano de tierra, CPWG 50 ohm,
                                   C101 en serie y una isla coaxial para NanoVNA
  - antenna-test-board.kicad_pro   proyecto (netclases RF/Default)
  - antenna-test-board.kicad_dru   regla DRC del gap coplanar del CPWG

El tamano de placa NO es arbitrario: 50 x 80 mm es la PCB de referencia de SJI
que consta en el User Manual del expediente FCC. En una IFA el plano de tierra
es el contrapeso y forma parte de la antena, asi que medir sobre el mismo plano
de la referencia es la unica forma de que el S11 sea comparable.
"""

from __future__ import annotations

import json
import re
import uuid
import sys
from pathlib import Path

import pcbnew
from pcbnew import VECTOR2I

import antenna_geometry as G

MM = 1_000_000


def v(x: float, y: float) -> VECTOR2I:
    return VECTOR2I(int(round(x * MM)), int(round(y * MM)))


def i(x: float) -> int:
    return int(round(x * MM))


# ---------------------------------------------------------------- parametros

BOARD_W = 50.00
BOARD_H = 80.00  # PCB de referencia SJI (User Manual FCC 5937666)

# Origen del footprint de la antena dentro de la placa.
# Deja la antena centrada en X y su borde superior a 4.00 mm del borde de placa.
ANT_ORG_X = (BOARD_W - G.ANT_W) / 2.0  # 5.25
ANT_ORG_Y = G.EDGE_TO_ANT_TOP  # 4.00

FEED_X = ANT_ORG_X + G.FEED_CENTER_X  # 39.25
PLANE_Y = ANT_ORG_Y + G.Y_GND  # 20.03

RF_W = G.FEED_W  # 1.00 mm  ancho del CPWG
RF_GAP = G.CPWG_GAP  # 0.15 mm gap coplanar

CAP_PITCH = 0.485
CAP_PAD = 0.60
# C101: el borde superior de su pad 1 cae a CPWG_LEN del borde del plano
C101_Y = PLANE_Y + G.CPWG_LEN + CAP_PAD / 2.0 + CAP_PITCH  # 26.415
C101_PAD1_Y = C101_Y - CAP_PITCH
C101_PAD2_Y = C101_Y + CAP_PITCH

COAX_Y = 30.50  # centro de la isla coaxial

VIA_PAD, VIA_DRILL = 0.60, 0.30
TRACK_DEFAULT = 0.25

CLEARANCE_RF = 0.15
CLEARANCE_MIN = 0.127  # minimo de fabricacion JLCPCB 2 capas
CLEARANCE_DEFAULT = 0.15  # = gap del CPWG (ver docs/verificacion.md)


# ---------------------------------------------------------------- utilidades

def add_net(board, name):
    net = pcbnew.NETINFO_ITEM(board, name)
    board.Add(net)
    return net


def add_edge_rect(board, x0, y0, x1, y1, width=0.10):
    for (ax, ay), (bx, by) in (
        ((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)),
        ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0)),
    ):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(v(ax, ay))
        s.SetEnd(v(bx, by))
        s.SetWidth(i(width))
        board.Add(s)


def add_track(board, x0, y0, x1, y1, width, net, layer=pcbnew.F_Cu):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(v(x0, y0))
    t.SetEnd(v(x1, y1))
    t.SetWidth(i(width))
    t.SetLayer(layer)
    t.SetNet(net)
    board.Add(t)
    return t


def add_via(board, x, y, net):
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(v(x, y))
    via.SetWidth(i(VIA_PAD))
    via.SetDrill(i(VIA_DRILL))
    via.SetViaType(pcbnew.VIATYPE_THROUGH)
    via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    via.SetNet(net)
    board.Add(via)
    return via


def add_text(board, text, x, y, layer, size=1.0, thickness=0.15, mirror=False):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetLayer(layer)
    t.SetPosition(v(x, y))
    t.SetTextSize(v(size, size))
    t.SetTextThickness(i(thickness))
    t.SetMirrored(mirror)
    board.Add(t)
    return t


def place(board, lib, name, ref, value, x, y, rot_deg=0.0, nickname=""):
    io = pcbnew.IO_MGR.PluginFind(pcbnew.IO_MGR.KICAD_SEXP)
    fp = io.FootprintLoad(str(lib), name)
    if fp is None:
        raise RuntimeError(f"no se pudo cargar {name} de {lib}")
    board.Add(fp)
    # el FPID tiene que llevar el nickname de la biblioteca, o DRC avisa de que
    # "la configuracion actual no incluye la biblioteca ''"
    if nickname:
        fp.SetFPID(pcbnew.LIB_ID(nickname, name))
    fp.SetReference(ref)
    fp.SetValue(value)
    if rot_deg:
        fp.SetOrientationDegrees(rot_deg)
    fp.SetPosition(v(x, y))
    return fp


def pad_of(fp, number):
    for p in fp.Pads():
        if p.GetNumber() == number:
            return p
    raise KeyError(number)


def add_gnd_zone(board, net, layer, rect, clearance):
    x0, y0, x1, y1 = rect
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net)
    z.SetZoneName(f"GND_{board.GetLayerName(layer)}")
    z.SetLocalClearance(i(clearance))
    z.SetMinThickness(i(0.20))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    z.SetIsFilled(False)
    o = z.Outline()
    o.NewOutline()
    for px, py in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
        o.Append(i(px), i(py))
    board.Add(z)
    return z


# ---------------------------------------------------------------- placa

def build(outdir: Path) -> None:
    outdir.mkdir(parents=True, exist_ok=True)
    libs = outdir.parent / "libraries"
    ant_lib, rf_lib = libs / "0G_Antenna.pretty", libs / "0G_RF.pretty"

    board = pcbnew.BOARD()
    board.SetCopperLayerCount(2)

    # ---- reglas de diseno ------------------------------------------------
    ds = board.GetDesignSettings()
    ds.SetBoardThickness(i(1.6))
    ds.m_MinClearance = i(CLEARANCE_MIN)
    ds.m_TrackMinWidth = i(CLEARANCE_MIN)
    ds.m_ViasMinSize = i(VIA_PAD)
    ds.m_MinThroughDrill = i(VIA_DRILL)
    ds.m_CopperEdgeClearance = i(0.25)

    # La netclase por defecto SI se puede fijar por API, y asi SaveBoard la
    # escribe en el .kicad_pro. Su clearance es 0.15 mm porque es el gap del
    # CPWG: en esta placa de prueba no hay nada que necesite mas.
    nc = ds.m_NetSettings.m_DefaultNetClass
    nc.SetClearance(i(CLEARANCE_DEFAULT))
    nc.SetTrackWidth(i(TRACK_DEFAULT))
    nc.SetViaDiameter(i(VIA_PAD))
    nc.SetViaDrill(i(VIA_DRILL))

    # ---- nets ------------------------------------------------------------
    gnd = add_net(board, "GND")
    ant_feed = add_net(board, "ANT_FEED")  # antena <-> C101
    rf_in = add_net(board, "RF_IN")  # C101 <-> isla coaxial

    # ---- contorno --------------------------------------------------------
    add_edge_rect(board, 0, 0, BOARD_W, BOARD_H)

    # ---- componentes -----------------------------------------------------
    ant = place(board, ant_lib, "ANT_IFA_915MHz_LSM110A", "ANT1",
                "ANT_IFA_915MHz_LSM110A", ANT_ORG_X, ANT_ORG_Y,
                nickname="0G_Antenna")
    pad_of(ant, "1").SetNet(ant_feed)
    pad_of(ant, "2").SetNet(gnd)

    c101 = place(board, rf_lib, "C_0402_1005Metric_0G", "C101", "2.2pF",
                 FEED_X, C101_Y, rot_deg=-90.0, nickname="0G_RF")
    pad_of(c101, "1").SetNet(ant_feed)
    pad_of(c101, "2").SetNet(rf_in)

    coax = place(board, rf_lib, "TP_Coax_50R_NanoVNA", "J1", "50R", FEED_X, COAX_Y,
                 nickname="0G_RF")
    pad_of(coax, "1").SetNet(gnd)
    pad_of(coax, "2").SetNet(rf_in)
    pad_of(coax, "3").SetNet(gnd)

    # ---- linea RF: CPWG 1.00 / 0.15 --------------------------------------
    # La pista arranca 0.60 mm por debajo del borde del plano para que su
    # casquete redondo (radio 0.50) quede fuera del keepout. El cobre no se
    # interrumpe: el ancla del pad de feed baja 1.50 mm por debajo del plano.
    rf_start_y = PLANE_Y + RF_W / 2.0 + 0.10

    # Junto a C101 la linea se estrecha al ancho de pad (0.60). Es lo que se hace
    # en RF para que el cobre no vuele por fuera del pad, y ademas evita que los
    # casquetes de dos pistas de 1.00 mm se toquen sobre un paso de 0.97 mm. El
    # tramo estrecho mide 0.63 mm = lambda/300 a 915 MHz: como discontinuidad de
    # impedancia es despreciable.
    neck = CAP_PAD  # 0.60
    neck_len = 0.63

    add_track(board, FEED_X, rf_start_y, FEED_X, C101_PAD1_Y - neck_len, RF_W, ant_feed)
    add_track(board, FEED_X, C101_PAD1_Y - neck_len, FEED_X, C101_PAD1_Y, neck, ant_feed)

    add_track(board, FEED_X, C101_PAD2_Y, FEED_X, C101_PAD2_Y + neck_len, neck, rf_in)
    add_track(board, FEED_X, C101_PAD2_Y + neck_len, FEED_X, COAX_Y, RF_W, rf_in)

    # ---- planos de tierra en las dos capas -------------------------------
    # El keepout del footprint de la antena recorta el relleno por encima de
    # PLANE_Y; no hace falta dibujar el hueco a mano.
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        add_gnd_zone(board, gnd, layer, (0, 0, BOARD_W, BOARD_H), CLEARANCE_RF)

    # ---- costura de vias -------------------------------------------------
    # 1) fila densa justo por debajo del borde del plano: en una IFA el borde
    #    del plano es parte de la antena y las dos capas deben estar al mismo
    #    potencial RF exactamente ahi.
    y_edge_row = PLANE_Y + 1.20
    x = 2.00
    while x <= BOARD_W - 2.00:
        if abs(x - FEED_X) > (RF_W / 2.0 + RF_GAP + VIA_PAD / 2.0 + 0.20):
            add_via(board, x, y_edge_row, gnd)
        x += 2.50

    # 2) vias coplanares a lo largo del CPWG, a ambos lados
    dx = RF_W / 2.0 + RF_GAP + VIA_PAD / 2.0 + 0.15  # 1.10 mm del eje
    y = y_edge_row + 2.00
    while y <= COAX_Y + 2.50:
        for sx in (-1, +1):
            add_via(board, FEED_X + sx * dx, y, gnd)
        y += 2.00

    # 3) costura perimetral, paso ~8 mm (lambda/20 a 915 MHz)
    per = 8.00
    y = PLANE_Y + 4.00
    while y <= BOARD_H - 2.50:
        add_via(board, 2.00, y, gnd)
        add_via(board, BOARD_W - 2.00, y, gnd)
        y += per
    x = 2.00 + per
    while x <= BOARD_W - 2.50:
        add_via(board, x, BOARD_H - 2.00, gnd)
        x += per

    # ---- serigrafia ------------------------------------------------------
    silk = [
        ("0G LockControl", 37.0, 1.6, 0.25),
        ("ANT_IFA_915MHz_LSM110A", 39.5, 1.1, 0.18),
        ("placa de prueba y ajuste v1", 41.5, 1.0, 0.15),
        ("ref. SJI 50x80 - FR4 1.6 - 2 capas", 44.5, 1.0, 0.15),
        ("Contains FCC ID: 2AS8LLSM110A", 47.0, 1.0, 0.15),
        ("Contains IC: 25119-LSM110A", 49.5, 1.0, 0.15),
        ("CPWG 50R 1.00/0.15", 53.0, 1.0, 0.15),
        ("J1 = pigtail coax -> NanoVNA", 55.5, 1.0, 0.15),
        ("20 cm min. antena-personas", 58.5, 1.0, 0.15),
    ]
    for txt, ty, tsize, tth in silk:
        add_text(board, txt, BOARD_W / 2, ty, pcbnew.F_SilkS, tsize, tth)

    add_text(board, "generado de antenna_geometry.py", BOARD_W / 2, 64.0,
             pcbnew.F_Fab, 1.0, 0.15)
    add_text(board, "no editar a mano", BOARD_W / 2, 66.5,
             pcbnew.F_Fab, 1.0, 0.15)

    # ---- guardar, recargar, rellenar, parchear ---------------------------
    # Tres detalles del comportamiento de pcbnew que marcan este orden:
    #  1. ZONE_FILLER revienta sobre un pcbnew.BOARD() suelto: necesita un
    #     PROJECT, que solo aparece al pasar por pcbnew.LoadBoard().
    #  2. SaveBoard reescribe el .kicad_pro desde las NET_SETTINGS en memoria,
    #     asi que la netclase por defecto se fija por API (arriba), no a mano.
    #  3. Dentro de un mismo proceso, LoadBoard reutiliza el PROJECT ya cacheado
    #     en lugar de releer el .kicad_pro del disco. Por eso la netclase RF y
    #     sus patrones -que KiCad 7 no expone por API- se parchean AL FINAL,
    #     cuando ya no queda ningun guardado que los pise.
    pcb = outdir / "antenna-test-board.kicad_pcb"
    pro = outdir / "antenna-test-board.kicad_pro"

    write_dru(outdir / "antenna-test-board.kicad_dru")
    write_fp_lib_table(outdir / "fp-lib-table")

    board.BuildListOfNets()
    pcbnew.SaveBoard(str(pcb), board)

    board2 = pcbnew.LoadBoard(str(pcb))
    got = board2.GetDesignSettings().m_NetSettings.m_DefaultNetClass.GetClearance() / MM
    if abs(got - CLEARANCE_DEFAULT) > 1e-6:
        raise RuntimeError(
            f"la netclase por defecto no se aplico: clearance = {got} mm, "
            f"esperado {CLEARANCE_DEFAULT} mm"
        )
    if not pcbnew.ZONE_FILLER(board2).Fill(board2.Zones()):
        raise RuntimeError("el relleno de zonas fallo")
    pcbnew.SaveBoard(str(pcb), board2)

    patch_project(pro)
    on_disk = json.loads(pro.read_text())["net_settings"]
    names = [c["name"] for c in on_disk["classes"]]
    if "RF" not in names:
        raise RuntimeError(f"la netclase RF no quedo en el proyecto: {names}")
    if len(on_disk["netclass_patterns"]) != 2:
        raise RuntimeError("faltan los patrones de netclase RF")

    n_blocks, n_uuid = canonicalize(pcb)

    print(f"escrito: {pcb}  ({n_blocks} bloques ordenados, {n_uuid} tstamp deterministas)")
    print(f"escrito: {pro}  (netclases: {', '.join(names)})")
    print(f"  zonas rellenadas : {len(list(board2.Zones()))}")
    print(f"  pads/vias/pistas : {len(list(board2.GetPads()))} / "
          f"{len([t for t in board2.GetTracks() if t.Type() == pcbnew.PCB_VIA_T])} / "
          f"{len([t for t in board2.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T])}")


UUID_NS = uuid.UUID("6f0a1c2e-9b3d-4f5a-8c7b-0d1e2f3a4b5c")  # el mismo de gen_footprints
UUID_RE = re.compile(
    r"\b[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\b"
)


# Orden canonico de los bloques de primer nivel. Los que no aparecen aqui se
# quedan en su sitio, al principio (version, generator, general, paper, layers,
# setup, net...): son la cabecera del archivo.
BLOCK_ORDER = {
    "footprint": 10,
    "gr_line": 20, "gr_rect": 21, "gr_circle": 22, "gr_arc": 23,
    "gr_poly": 24, "gr_curve": 25, "gr_text": 26,
    "dimension": 30,
    "segment": 40, "arc": 41, "via": 42,
    "zone": 50,
    "group": 60,
}

# Lo mismo dentro de cada `(footprint ...)`. Lo demas (layer, tstamp, at, descr,
# tags, attr, net_tie_pad_groups, property...) es cabecera y se queda en su sitio.
FP_CHILD_ORDER = {
    "fp_text": 10,
    "fp_line": 20, "fp_rect": 21, "fp_circle": 22, "fp_arc": 23,
    "fp_poly": 24, "fp_curve": 25,
    "pad": 30,
    "zone": 40,
    "group": 50,
    "model": 60,
}


def _split_forms(body: str) -> list[str]:
    """
    Parte un cuerpo de s-expresiones en sus formas hijas de primer nivel.

    Cada forma se devuelve con su indentacion de linea incluida, para que al
    reordenarlas y volver a unirlas con "\\n" el archivo mantenga el mismo
    formato que escribio KiCad. Respeta las cadenas entre comillas, asi que un
    parentesis dentro de un texto no descuadra el conteo.
    """
    out, depth, start, in_str, esc = [], 0, None, False, False
    for i, ch in enumerate(body):
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
            continue
        if ch == '"':
            in_str = True
        elif ch == "(":
            if depth == 0:
                # retrocede para incluir la indentacion de la linea
                j = i
                while j > 0 and body[j - 1] in " \t":
                    j -= 1
                start = j
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0 and start is not None:
                out.append(body[start:i + 1])
                start = None
    return out


def _form_name(form: str) -> str:
    return form.lstrip()[1:].split(None, 1)[0].strip("()")


def _canon_form(block: str, order: dict[str, int]) -> str:
    """Reordena las formas hijas de `block` que aparezcan en `order`."""
    op = block.index("(")
    close = block.rindex(")")
    nl = block.find("\n", op)
    if nl == -1 or nl > close:
        return block  # forma de una sola linea: nada que ordenar

    prologue, body, tail = block[:nl + 1], block[nl + 1:close], block[close:]
    children = _split_forms(body)
    sortable = [c for c in children if _form_name(c) in order]
    if not sortable:
        return block

    header = [c for c in children if _form_name(c) not in order]
    sortable.sort(key=lambda c: (order[_form_name(c)], UUID_RE.sub("", c)))
    return prologue + "\n".join(header + sortable) + "\n" + tail


def canonicalize(pcb: Path) -> tuple[int, int]:
    """
    Deja el .kicad_pcb en una forma canonica, para que build.sh sea reproducible
    byte a byte y el diff de git de la placa sirva para revisar cambios reales.

    Hacen falta dos pasos, por dos comportamientos distintos de pcbnew:

    1. ORDEN. Al guardar, KiCad ordena los footprints y demas items con un
       criterio que acaba dependiendo de sus KIID. Como esos KIID son aleatorios
       en el primer guardado, el orden de los bloques cambia en cada ejecucion.
       Se reordenan aqui por (tipo, texto sin UUID), que no depende de nada
       aleatorio. El orden de los hijos de `(kicad_pcb ...)` no tiene
       significado semantico, asi que reordenarlos es seguro; de todos modos
       verify_board.py vuelve a cargar el archivo y corre el DRC despues.

    2. UUID. pcbnew asigna un KIID aleatorio a cada item y `m_Uuid` es de solo
       lectura desde Python, asi que no se pueden fijar al construir. Se
       sustituyen por uuid5 deterministas, por orden de primera aparicion en el
       texto YA ordenado. Todas las apariciones de un mismo UUID van al mismo
       valor, asi que las referencias cruzadas del archivo siguen siendo validas.

    Devuelve (bloques reordenados, UUID sustituidos).
    """
    text = pcb.read_text(encoding="utf-8")

    # KiCad reordena tambien los hijos DENTRO de cada footprint (los fp_text de
    # documentacion, en concreto), asi que hay que canonicalizar los dos niveles.
    text = _canon_form(text, BLOCK_ORDER)

    op = text.index("(kicad_pcb")
    close = text.rindex(")")
    body = text[text.index("\n", op) + 1:close]
    blocks = _split_forms(body)
    if not blocks:
        raise RuntimeError("no se pudieron separar los bloques del .kicad_pcb")

    n_sorted = 0
    for block in blocks:
        if _form_name(block) == "footprint":
            canon = _canon_form(block, FP_CHILD_ORDER)
            if canon != block:
                text = text.replace(block, canon, 1)
            n_sorted += 1

    mapping: dict[str, str] = {}
    for found in UUID_RE.findall(text):
        if found not in mapping:
            mapping[found] = str(uuid.uuid5(UUID_NS, f"test-board:{len(mapping)}"))

    pcb.write_text(UUID_RE.sub(lambda m: mapping[m.group(0)], text), encoding="utf-8")
    return len(blocks), len(mapping)


FP_LIB_TABLE = """(fp_lib_table
  (version 7)
  (lib (name "0G_Antenna")(type "KiCad")(uri "${KIPRJMOD}/../libraries/0G_Antenna.pretty")(options "")(descr "Antenas PCB de 0G LockControl"))
  (lib (name "0G_RF")(type "KiCad")(uri "${KIPRJMOD}/../libraries/0G_RF.pretty")(options "")(descr "Componentes de apoyo RF de 0G LockControl"))
)
"""


def write_fp_lib_table(path: Path) -> None:
    path.write_text(FP_LIB_TABLE, encoding="utf-8")
    print(f"escrito: {path}")


def patch_project(path: Path) -> None:
    """
    Anade al .kicad_pro que escribio KiCad las netclases Default y RF.

    Se parchea en vez de escribirlo desde cero: asi nunca se pelea con el
    esquema JSON de la version de KiCad que toque, que es lo que hacia que
    KiCad descartara el archivo y lo reemplazara por sus valores por defecto.
    """
    proj = json.loads(path.read_text())
    ns = proj.setdefault("net_settings", {})
    classes = ns.setdefault("classes", [])

    base = classes[0] if classes else {}

    def cls(name, clearance, track, color):
        d = dict(base)
        d.update({
            "name": name, "clearance": clearance, "track_width": track,
            "via_diameter": VIA_PAD, "via_drill": VIA_DRILL, "pcb_color": color,
        })
        return d

    # Default ya lo escribio KiCad con los valores fijados por API; aqui solo
    # se anade RF, que KiCad 7 no permite crear desde Python.
    keep = [c for c in classes if c.get("name") == "Default"] or [
        cls("Default", CLEARANCE_DEFAULT, TRACK_DEFAULT, "rgba(0, 0, 0, 0.000)")]
    ns["classes"] = keep + [cls("RF", CLEARANCE_RF, RF_W, "rgba(255, 0, 0, 0.700)")]
    ns["netclass_patterns"] = [
        {"netclass": "RF", "pattern": "ANT_FEED"},
        {"netclass": "RF", "pattern": "RF_IN"},
    ]
    path.write_text(json.dumps(proj, indent=2) + "\n", encoding="utf-8")


DRU = """(version 1)

# Reglas DRC especificas de RF para la antena IFA del LSM110A.
#
# El gap coplanar del CPWG es un parametro de impedancia, no una holgura de
# fabricacion: con 1.00 mm de pista, 0.15 mm de gap, FR4 de 1.6 mm y er = 4.3
# la linea da ~50 ohm (User Manual FCC 5937666). Si el gap crece, la impedancia
# sube y aparece desadaptacion.

(rule "CPWG 50R - gap coplanar de la linea RF"
    (condition "A.NetClass == 'RF' && B.NetClass != 'RF'")
    (constraint clearance (min 0.15mm)))

(rule "RF - sin costura de vias dentro del area de antena"
    (condition "A.Type == 'Via' && A.insideArea('ANT_KEEPOUT')")
    (constraint disallow via))
"""


def write_dru(path: Path) -> None:
    path.write_text(DRU, encoding="utf-8")
    print(f"escrito: {path}")


if __name__ == "__main__":
    build(Path(sys.argv[1] if len(sys.argv) > 1 else "../antenna-test-board"))
