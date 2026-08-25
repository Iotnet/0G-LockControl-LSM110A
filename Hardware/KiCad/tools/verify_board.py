#!/usr/bin/env python3
"""
Verificacion de la placa de prueba, midiendo el cobre REAL que produce KiCad.

Corre el DRC y despues mide sobre la geometria ya rellenada: donde arranca el
plano de tierra, cuanto vale el gap coplanar del CPWG, donde caen los pads de
C101. Son las cotas que deciden si la antena resuena donde debe, asi que se
comprueban sobre el resultado, no sobre la intencion.

    python3 verify_board.py ../antenna-test-board/antenna-test-board.kicad_pcb
"""

from __future__ import annotations

import sys
from pathlib import Path

import pcbnew

import antenna_geometry as G

MM = 1_000_000.0
TOL = 0.002

results: list[tuple[bool, str]] = []


def check(cond, label, detail=""):
    results.append((bool(cond), f"{label}{(' -> ' + detail) if detail else ''}"))
    return bool(cond)


def close(got, want, label, tol=TOL):
    return check(abs(got - want) <= tol, label, f"{got:.4f} (esperado {want:.4f})")


# Coordenadas absolutas esperadas en la placa
ANT_ORG_X = (50.0 - G.ANT_W) / 2.0  # 5.25
ANT_ORG_Y = G.EDGE_TO_ANT_TOP  # 4.00
PLANE_Y = ANT_ORG_Y + G.Y_GND  # 20.03
FEED_X = ANT_ORG_X + G.FEED_CENTER_X  # 39.25


def poly_of(zone, layer):
    return zone.GetFilledPolysList(layer)


def main() -> None:
    pcb = Path(sys.argv[1] if len(sys.argv) > 1
               else "../antenna-test-board/antenna-test-board.kicad_pcb").resolve()
    board = pcbnew.LoadBoard(str(pcb))

    print("=" * 78)
    print(f"Verificacion de la placa: {pcb.name}")
    print("=" * 78)

    # ---- DRC -------------------------------------------------------------
    rpt = pcb.parent.parent / "export" / "drc-report.txt"
    rpt.parent.mkdir(parents=True, exist_ok=True)
    pcbnew.WriteDRCReport(board, str(rpt), pcbnew.EDA_UNITS_MILLIMETRES, True)
    text = rpt.read_text()
    n_drc = int(text.split("Found ")[1].split(" DRC")[0])
    n_unc = int(text.split("Found ")[2].split(" unconnected")[0])
    n_fp = int(text.split("Found ")[3].split(" Footprint")[0])
    check(n_drc == 0, "DRC: 0 violaciones", str(n_drc))
    check(n_unc == 0, "DRC: 0 pads sin conectar", str(n_unc))
    check(n_fp == 0, "DRC: 0 errores de footprint", str(n_fp))

    # ---- contorno --------------------------------------------------------
    # El bounding box de Edge.Cuts incluye el ancho de linea; el contorno real
    # de la placa es la LINEA MEDIA de esa geometria, asi que hay que descontarlo.
    edges = [d for d in board.GetDrawings() if d.GetLayer() == pcbnew.Edge_Cuts]
    check(len(edges) == 4, "contorno: 4 tramos en Edge.Cuts", str(len(edges)))
    ew = max(d.GetWidth() for d in edges) / MM
    bb = board.GetBoardEdgesBoundingBox()
    close(bb.GetWidth() / MM - ew, 50.0, "contorno: ancho de placa (linea media)")
    close(bb.GetHeight() / MM - ew, 80.0,
          "contorno: alto de placa (linea media, referencia SJI 50x80)")

    # ---- donde arranca el plano de tierra --------------------------------
    # Es LA cota critica: en una IFA el borde del plano es parte de la antena.
    zones = {board.GetLayerName(z.GetLayer()): z for z in board.Zones()}
    check(set(zones) == {"F.Cu", "B.Cu"}, "hay plano de tierra en las dos capas",
          str(sorted(zones)))

    for lname, z in sorted(zones.items()):
        check(z.GetNetname() == "GND", f"{lname}: el plano es de la red GND",
              z.GetNetname())
        fill = poly_of(z, z.GetLayer())
        check(fill.OutlineCount() >= 1, f"{lname}: el plano tiene relleno")
        top = fill.BBox().GetTop() / MM
        close(top, PLANE_Y, f"{lname}: el relleno arranca en y = {PLANE_Y}")

        # ningun punto del plano por encima del borde: se comprueba muestreando
        # el area de la antena en una rejilla
        intrusos = 0
        y = -3.0
        while y < PLANE_Y - 0.05:
            x = 1.0
            while x < 49.0:
                if fill.Contains(pcbnew.VECTOR2I(int(x * MM), int(y * MM))):
                    intrusos += 1
                x += 0.5
            y += 0.5
        check(intrusos == 0,
              f"{lname}: 0 puntos de plano dentro del area de antena "
              f"(rejilla de 0.5 mm)", f"{intrusos} intrusos")

    # ---- gap coplanar real del CPWG --------------------------------------
    # Se busca el borde del plano a media altura de la linea, barriendo en X.
    fill_f = poly_of(zones["F.Cu"], pcbnew.F_Cu)
    y_probe = PLANE_Y + 3.0  # dentro del tramo de 5.60 mm, lejos de vias
    step = 0.001
    x = FEED_X + G.FEED_W / 2.0
    while x < FEED_X + 3.0:
        if fill_f.Contains(pcbnew.VECTOR2I(int(x * MM), int(y_probe * MM))):
            break
        x += step
    gap = x - (FEED_X + G.FEED_W / 2.0)
    close(gap, G.CPWG_GAP, "gap coplanar medido a la derecha del CPWG", tol=0.005)

    x = FEED_X - G.FEED_W / 2.0
    while x > FEED_X - 3.0:
        if fill_f.Contains(pcbnew.VECTOR2I(int(x * MM), int(y_probe * MM))):
            break
        x -= step
    gap_l = (FEED_X - G.FEED_W / 2.0) - x
    close(gap_l, G.CPWG_GAP, "gap coplanar medido a la izquierda del CPWG", tol=0.005)

    # ---- footprints ------------------------------------------------------
    fps = {f.GetReference(): f for f in board.GetFootprints()}
    check(set(fps) == {"ANT1", "C101", "J1"}, "componentes en la placa",
          str(sorted(fps)))

    ant = fps["ANT1"]
    pads = {p.GetNumber(): p for p in ant.Pads()}
    check(pads["1"].GetNetname() == "ANT_FEED", "ANT1 pad 1 en la red ANT_FEED",
          pads["1"].GetNetname())
    check(pads["2"].GetNetname() == "GND", "ANT1 pad 2 (stub) en la red GND",
          pads["2"].GetNetname())

    cu = pads["1"].GetEffectivePolygon().BBox()
    close(cu.GetLeft() / MM, ANT_ORG_X, "cobre de antena: borde izquierdo absoluto")
    close(cu.GetRight() / MM, ANT_ORG_X + G.ANT_W, "cobre de antena: borde derecho absoluto")
    close(cu.GetTop() / MM, ANT_ORG_Y, "cobre de antena: borde superior absoluto")
    close(ANT_ORG_Y - 0.0, G.EDGE_TO_ANT_TOP,
          "cobre de antena: 4.00 mm por debajo del borde de placa")

    # C101: la rotacion tiene que dejar el pad 1 hacia la antena
    c = fps["C101"]
    cp = {p.GetNumber(): p for p in c.Pads()}
    check(cp["1"].GetNetname() == "ANT_FEED", "C101 pad 1 hacia la antena (ANT_FEED)",
          cp["1"].GetNetname())
    check(cp["2"].GetNetname() == "RF_IN", "C101 pad 2 hacia el radio (RF_IN)",
          cp["2"].GetNetname())
    check(cp["1"].GetPosition().y < cp["2"].GetPosition().y,
          "C101 pad 1 queda arriba (mas cerca de la antena)")
    close(cp["1"].GetBoundingBox().GetTop() / MM, PLANE_Y + G.CPWG_LEN,
          f"C101: borde del pad 1 a {G.CPWG_LEN} mm del plano (cota 5.60)")

    # ---- vias ------------------------------------------------------------
    vias = [t for t in board.GetTracks() if t.Type() == pcbnew.PCB_VIA_T]
    check(len(vias) > 20, "hay costura de vias", f"{len(vias)} vias")
    check(all(t.GetNetname() == "GND" for t in vias), "todas las vias son de GND")
    dentro = [t for t in vias
              if (t.GetPosition().y / MM - t.GetWidth() / MM / 2.0) < PLANE_Y]
    check(not dentro, "0 vias dentro del area de antena", f"{len(dentro)} vias")

    # ---- pistas RF -------------------------------------------------------
    tracks = [t for t in board.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T]
    rf = [t for t in tracks if t.GetNetname() in ("ANT_FEED", "RF_IN")]
    check(len(rf) == len(tracks), "todas las pistas de la placa son la linea RF",
          f"{len(rf)}/{len(tracks)}")
    anchos = sorted({round(t.GetWidth() / MM, 3) for t in rf})
    check(anchos == [0.6, 1.0], "anchos de la linea RF: 1.00 (CPWG) y 0.60 (cuello)",
          str(anchos))
    top_rf = min(t.GetStart().y for t in rf) / MM
    check(top_rf - 0.5 >= PLANE_Y - TOL,
          "la pista RF arranca fuera del keepout (con su casquete)",
          f"y={top_rf:.3f}, casquete hasta {top_rf - 0.5:.3f}")

    # ---- salida ----------------------------------------------------------
    for ok, label in results:
        print(f"[{'OK ' if ok else 'FALLA'}] {label}")
    n_bad = sum(1 for ok, _ in results if not ok)
    print()
    print(f"placa: {len(results) - n_bad} OK, {n_bad} FALLA")
    sys.exit(0 if n_bad == 0 else 1)


if __name__ == "__main__":
    main()
