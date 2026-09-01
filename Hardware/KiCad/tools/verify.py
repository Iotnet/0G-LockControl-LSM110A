#!/usr/bin/env python3
"""
Verificacion de los footprints generados, usando el motor real de KiCad (pcbnew).

No comprueba "que el archivo exista": lo carga con el mismo parser que usa KiCad y
mide la geometria resultante contra antenna_geometry.py.

    python3 verify.py [carpeta]

Sin argumento verifica antenna_geometry.LIB_DIR, o sea
`Hardware/v0-replica-sji/kicad-lib/`.

Codigo de salida 0 si todo pasa, 1 si algo falla.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pcbnew

import antenna_geometry as G

MM = 1_000_000.0
TOL_MM = 0.002  # 2 um: por debajo de cualquier tolerancia de fabricacion

results: list[tuple[bool, str]] = []


def check(cond: bool, label: str, detail: str = "") -> bool:
    results.append((bool(cond), f"{label}{(' -> ' + detail) if detail else ''}"))
    return bool(cond)


def close(got: float, want: float, label: str, tol: float = TOL_MM) -> bool:
    return check(abs(got - want) <= tol, label, f"{got:.4f} (esperado {want:.4f})")


def pad_area_mm2(pad) -> float:
    """Area efectiva del cobre de un pad, via su poligono efectivo."""
    shape = pad.GetEffectivePolygon()
    return shape.Area() / (MM * MM)


def pad_bbox_mm(pad):
    b = pad.GetBoundingBox()
    return (b.GetLeft() / MM, b.GetTop() / MM, b.GetRight() / MM, b.GetBottom() / MM)


def verify_antenna(lib: Path) -> None:
    name = "ANT_IFA_915MHz_LSM110A"
    io = pcbnew.IO_MGR.PluginFind(pcbnew.IO_MGR.KICAD_SEXP)
    fp = io.FootprintLoad(str(lib), name)

    if not check(fp is not None, f"{name}: KiCad carga el footprint sin errores"):
        return

    # ---- pads ------------------------------------------------------------
    pads = {p.GetNumber(): p for p in fp.Pads()}
    check(set(pads) == {"1", "2"}, "pads presentes = {1 FEED, 2 GND}", str(sorted(pads)))

    # net tie declarado: KiCad debe reconocer que 1 y 2 estan unidos a proposito
    check(fp.IsNetTie(), "el footprint es un net-tie reconocido por KiCad")
    tied = sorted(p.GetNumber() for p in fp.GetNetTiePads(pads["1"]))
    check(tied == ["1", "2"],
          "KiCad resuelve pad 1 y pad 2 como cortocircuito intencional (net tie)",
          str(tied))

    # ---- geometria del cobre --------------------------------------------
    p1 = pads["1"]
    # Sobre el area del plano, el pad 1 anade su ancla: 1.00 mm de ancho que
    # baja FEED_ANCHOR_DOWN por debajo del borde del plano, para que la pista
    # CPWG pueda arrancar fuera del keepout. Ese cobre extra cae sobre el
    # trazado de la linea de feed, que en el plano sigue recta hasta C101.
    close(pad_area_mm2(p1), G.copper_area() + G.FEED_ANCHOR_EXTRA_AREA,
          "area de cobre del pad 1 == area del plano (+ ancla del feed)", tol=0.01)

    x0, y0, x1, y1 = pad_bbox_mm(p1)
    close(x0, 0.0, "bbox cobre: borde izquierdo en x = 0")
    close(x1, G.ANT_W, f"bbox cobre: borde derecho en x = {G.ANT_W}")
    close(y0, 0.0, "bbox cobre: borde superior en y = 0")
    close(y1, G.Y_GND + G.FEED_ANCHOR_DOWN,
          "bbox cobre: extremo inferior = borde del plano + ancla del feed")
    check(G.FEED_ANCHOR_DOWN >= G.FEED_W / 2.0 + 0.10,
          "el ancla del feed deja arrancar la pista CPWG fuera del keepout",
          f"{G.FEED_ANCHOR_DOWN} mm >= {G.FEED_W / 2.0 + 0.10} mm")

    # el cobre esta SOLO en F.Cu -> queda cubierto por la mascara de soldadura
    lset = list(p1.GetLayerSet().Seq())
    check(lset == [pcbnew.F_Cu], "pad 1 solo en F.Cu (cobre bajo mascara, sin pasta)",
          str([pcbnew.BOARD().GetLayerName(l) for l in lset]))
    lset2 = list(pads["2"].GetLayerSet().Seq())
    check(lset2 == [pcbnew.F_Cu], "pad 2 solo en F.Cu")
    check(pads["2"].GetZoneConnection() == pcbnew.ZONE_CONNECTION_FULL,
          "pad 2 conecta al plano en solido (sin alivio termico)")

    # ---- puntos clave dentro / fuera del cobre --------------------------
    poly = p1.GetEffectivePolygon()

    def inside(x, y):
        return poly.Contains(pcbnew.VECTOR2I(int(x * MM), int(y * MM)))

    probes_in = [
        ("centro brazo superior", 20.0, 2.5),
        ("centro brazo central", 20.0, 8.0),
        ("centro brazo inferior", 17.0, 12.0),
        ("puente derecho", 36.75, 5.25),
        ("puente izquierdo", 3.0, 10.75),
        ("stub L horizontal", 37.0, 12.5),
        ("stub L vertical", 39.0, 14.5),
        ("linea de feed", 34.0, 14.5),
    ]
    probes_out = [
        ("ranura 1 (extremo abierto izq)", 1.0, 5.25),
        ("ranura 1 (centro)", 20.0, 5.25),
        ("ranura 2 (centro)", 20.0, 10.75),
        ("ranura 2 (extremo abierto der)", 38.0, 10.75),
        ("hueco a la derecha del brazo inferior", 37.0, 11.5),
        ("hueco entre feed y stub", 36.0, 14.5),
        ("fuera: por encima de la antena", 20.0, -1.0),
        ("fuera: a la derecha de la antena", 40.5, 5.0),
    ]
    for label, x, y in probes_in:
        check(inside(x, y), f"CON cobre en ({x}, {y}) - {label}")
    for label, x, y in probes_out:
        check(not inside(x, y), f"SIN cobre en ({x}, {y}) - {label}")

    # ---- canto superior del tramo horizontal del stub --------------------
    # Es LA cota que estaba mal (se suponia 1.00 de alto -> 4.03; el plano dice
    # 4.05 -> 1.02). Las sondas de arriba no la encierran, asi que se BUSCA el
    # canto barriendo en y, en una x donde el unico cobre posible es el stub
    # (a la derecha del brazo inferior, que acaba en 34.50).
    x_stub = 37.0
    y, step = 11.00, 0.001
    while y < 13.00 and not inside(x_stub, y):
        y += step
    close(y, G.STUB_TOP_Y,
          f"canto superior del tramo del stub medido en x={x_stub}", tol=0.0015)
    close(G.Y_GND - y, 4.05,
          "extension vertical del stub = cota 4.05 del plano", tol=0.0015)
    close(G.Y_BOT - y, 1.02,
          "alto del tramo horizontal del stub = 4.05 - 3.03", tol=0.0015)

    # ---- conectividad de la forma de "2" --------------------------------
    # Un solo poligono relleno, sin huecos: eso prueba que los dos puentes unen
    # los tres brazos y que ninguna ranura corta la pieza en dos.
    check(poly.OutlineCount() == 1,
          "el cobre es UN solo poligono conexo", f"contornos = {poly.OutlineCount()}")
    check(poly.HoleCount(0) == 0,
          "el cobre no tiene huecos (ambas ranuras abren a un borde)",
          f"huecos = {poly.HoleCount(0)}")

    # ---- keepout ---------------------------------------------------------
    zones = list(fp.Zones())
    if check(len(zones) == 1, "el footprint lleva 1 rule area (keepout)", str(len(zones))):
        z = zones[0]
        check(z.GetIsRuleArea(), "la zona es rule area (no un relleno de cobre)")
        check(z.GetDoNotAllowCopperPour(), "keepout: prohibe relleno de plano")
        check(z.GetDoNotAllowVias(), "keepout: prohibe vias")
        check(z.GetDoNotAllowTracks(), "keepout: prohibe pistas")
        check(not z.GetDoNotAllowPads(),
              "keepout: PERMITE pads (si no, marcaria el cobre de la propia antena)")
        zl = list(z.GetLayerSet().Seq())
        check(zl == [pcbnew.F_Cu, pcbnew.B_Cu], "keepout en F.Cu y B.Cu",
              str(len(zl)) + " capas")
        b = z.GetBoundingBox()
        close(b.GetLeft() / MM, G.KEEPOUT[0], "keepout: borde izquierdo")
        close(b.GetTop() / MM, G.KEEPOUT[1], "keepout: borde superior")
        close(b.GetRight() / MM, G.KEEPOUT[2], "keepout: borde derecho")
        close(b.GetBottom() / MM, G.KEEPOUT[3], "keepout: borde inferior = borde del plano")

    # ---- courtyard -------------------------------------------------------
    crtyd = [g for g in fp.GraphicalItems()
             if g.GetLayer() == pcbnew.F_CrtYd]
    check(len(crtyd) == 1, "hay courtyard en F.CrtYd", str(len(crtyd)))

    # ---- atributos -------------------------------------------------------
    a = fp.GetAttributes()
    check(bool(a & pcbnew.FP_EXCLUDE_FROM_BOM), "excluido del BOM (es cobre, no un componente)")
    check(bool(a & pcbnew.FP_EXCLUDE_FROM_POS_FILES), "excluido del fichero de posiciones")


def verify_dimensions(lib: Path) -> None:
    """
    Las cotas dibujadas en Cmts.User tienen que caer sobre la geometria que
    dicen medir. Si alguien mueve una cota del plano y se olvida del dibujo, la
    anotacion mentiria -- y una anotacion que miente es peor que no tenerla.
    """
    io = pcbnew.IO_MGR.PluginFind(pcbnew.IO_MGR.KICAD_SEXP)
    fp = io.FootprintLoad(str(lib), "ANT_IFA_915MHz_LSM110A")
    if fp is None:
        return

    marks = [g for g in fp.GraphicalItems()
             if g.GetLayer() == pcbnew.Cmts_User and g.GetClass() == "MGRAPHIC"
             and abs(g.GetStart().y - g.GetEnd().y) < 1000]  # testigos horizontales
    ys = sorted({round(g.GetStart().y / MM, 3) for g in marks})

    for want, label in ((G.Y_BOT, "3.03: testigo en el fondo del cobre (13.00)"),
                        (G.STUB_TOP_Y, "4.05: testigo en el techo del stub (11.98)"),
                        (G.Y_GND, "3.03 y 4.05: testigo en el borde del plano (16.03)")):
        hit = min(ys, key=lambda v: abs(v - want)) if ys else float("nan")
        check(abs(hit - want) <= 0.002, label, f"testigo mas cercano en y = {hit}")

    # los dos valores rotulados salen de la geometria, no del texto
    close(G.Y_GND - G.Y_BOT, 3.03, "cota rotulada 3.03 = Y_GND - Y_BOT")
    close(G.Y_GND - G.STUB_TOP_Y, 4.05, "cota rotulada 4.05 = Y_GND - STUB_TOP_Y")

    # Y NO puede haber cotas por debajo del plano: ahi el footprint no tiene
    # cobre, asi que cualquier cota apuntaria al vacio. La 5.60 va en la placa.
    huerfanas = [y for y in ys if y > G.Y_GND + 0.01]
    check(not huerfanas,
          "sin cotas por debajo del plano (la 5.60 va en la placa, no aqui)",
          f"{len(huerfanas)} testigo(s) en y = {huerfanas}")

    # ---- la cota tiene que LLEGAR al cobre que mide ----------------------
    # Este es el defecto que se colo dos veces: la cota estaba en su sitio y
    # medía lo correcto, pero flotaba a 1.7 mm del cobre sin linea de extension
    # que las uniera, asi que no se veia que estaba midiendo. Que una cota este
    # bien situada no basta: tiene que verse de donde sale.
    poly = next(p for p in fp.Pads() if p.GetNumber() == "1").GetEffectivePolygon()

    def hay_cobre(x, y, hacia, alcance=0.30):
        paso = 0.01
        n = int(alcance / paso)
        return any(poly.Contains(pcbnew.VECTOR2I(int((x + hacia * k * paso) * MM),
                                                 int(y * MM))) for k in range(n + 1))

    # Se leen los extremos REALES de la linea de extension del fichero. Una
    # version anterior de esta comprobacion los daba por supuestos con una
    # constante, y por eso no detectaba nada: comprobaba su propia suposicion.
    cu = next(p for p in fp.Pads() if p.GetNumber() == "1").GetBoundingBox()
    cx = cu.Centre().x / MM
    horiz = [(g.GetStart().x / MM, g.GetEnd().x / MM, g.GetStart().y / MM)
             for g in marks]

    # dy: a que lado del canto esta el cobre (13.00 es borde inferior del brazo
    # -> el cobre queda arriba; 11.98 es borde superior del tramo del stub ->
    # el cobre queda abajo)
    for label, y, dy in (("3.03", G.Y_BOT, -0.01), ("4.05", G.STUB_TOP_Y, +0.01)):
        tramos = [(a, b) for a, b, yy in horiz if abs(yy - y) < 0.002]
        if not check(tramos, f"{label}: hay linea de extension en y = {y}"):
            continue
        # extremo interior = el mas cercano al cobre; se prueba hacia el cobre
        a, b = tramos[0]
        x_in = min((a, b), key=lambda v: abs(v - cx))
        hacia = 1.0 if cx > x_in else -1.0
        check(hay_cobre(x_in, y + dy, hacia),
              f"{label}: la linea de extension LLEGA al cobre que mide",
              f"extremo interior en x = {x_in:.2f}; cobre a "
              f"{'<=' if hay_cobre(x_in, y + dy, hacia) else '>'}0.30 mm")


def verify_slotrow(lib: Path) -> None:
    name = "ANT_LSM110A_BreakAwaySlots_EVM_ONLY"
    io = pcbnew.IO_MGR.PluginFind(pcbnew.IO_MGR.KICAD_SEXP)
    fp = io.FootprintLoad(str(lib), name)
    if not check(fp is not None, f"{name}: KiCad carga el footprint sin errores"):
        return

    edge = [g for g in fp.GraphicalItems() if g.GetLayer() == pcbnew.Edge_Cuts]
    check(len(edge) == 4 * len(G.SLOTROW_X),
          f"{len(G.SLOTROW_X)} ranuras x 4 tramos en Edge.Cuts", str(len(edge)))
    check(len(list(fp.Pads())) == 0, "la fila de ranuras no lleva pads")

    # cada ranura debe cerrar: los extremos de los 4 tramos coinciden por pares
    for gname, sx0, sx1 in G.SLOTROW_X:
        segs = [g for g in edge
                if sx0 - 0.01 <= g.GetBoundingBox().Centre().x / MM <= sx1 + 0.01]
        check(len(segs) == 4, f"ranura {gname}: 4 tramos", str(len(segs)))

    # clearance al cobre de la antena
    for gname, sx0, sx1 in G.SLOTROW_X:
        for cu_x0, cu_x1, what in ((G.FEED_X0, G.FEED_X1, "feed"),
                                   (G.STUB_LEG_X0, G.STUB_LEG_X1, "stub")):
            overlap = min(sx1, cu_x1) - max(sx0, cu_x0)
            check(overlap <= -0.25,
                  f"ranura {gname} deja >= 0.25 mm al cobre del {what}",
                  f"holgura = {-overlap:.2f} mm" if overlap < 0 else f"SOLAPE {overlap:.2f} mm")


def main() -> None:
    lib = (Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else G.lib_dir(__file__))

    print("=" * 78)
    print("Verificacion de geometria (antenna_geometry.py)")
    print("=" * 78)
    geom = G.check()
    for line in geom:
        print(line)
    geom_ok = all("FALLA" not in ln for ln in geom)

    print()
    print("=" * 78)
    print(f"Verificacion de footprints con pcbnew {pcbnew.GetBuildVersion()}")
    print(f"biblioteca: {lib}")
    print("=" * 78)
    verify_antenna(lib)
    print()
    verify_dimensions(lib)
    print()
    verify_slotrow(lib)

    for ok, label in results:
        print(f"[{'OK ' if ok else 'FALLA'}] {label}")

    n_ok = sum(1 for ok, _ in results if ok)
    n_bad = len(results) - n_ok
    print()
    print(f"geometria: {'OK' if geom_ok else 'FALLA'}   footprints: {n_ok} OK, {n_bad} FALLA")
    sys.exit(0 if (geom_ok and n_bad == 0) else 1)


if __name__ == "__main__":
    main()
