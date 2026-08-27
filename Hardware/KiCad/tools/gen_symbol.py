#!/usr/bin/env python3
"""
Genera la biblioteca de simbolos 0G_Antenna.kicad_sym.

    python3 gen_symbol.py [archivo]

Sin argumento escribe en antenna_geometry.LIB_DIR / SYM_LIB_FILE, o sea
`Hardware/v0-replica-sji/kicad-lib/0G_Antenna.kicad_sym`: la misma carpeta que
el LSM110A.

El campo Footprint apunta a `0G-LockControl:ANT_IFA_915MHz_LSM110A`, que es el
nickname con el que el README de kicad-lib registra esa carpeta. Si registras la
carpeta con otro nickname, el enlace simbolo->footprint hay que rehacerlo a mano.

El simbolo tiene DOS pines, y eso no es un capricho: una IFA esta unida a masa
por su stub de cortocircuito, asi que en el esquematico hay que dibujar ese
camino. El footprint declara los pads 1 y 2 como net tie, y KiCad entiende
entonces que el corto es intencional y no lo marca en el DRC.

    pin 1  FEED  -> va a C101 y de ahi al RFOUT del modulo
    pin 2  GND   -> stub de cortocircuito al plano de tierra
"""

from __future__ import annotations

import sys
from pathlib import Path

import antenna_geometry as G

LIB = f"""(kicad_symbol_lib (version 20220914) (generator 0g_antenna_gen)
  (symbol "ANT_IFA_915MHz_LSM110A" (pin_names (offset 0.762) hide) (in_bom no) (on_board yes)
    (property "Reference" "ANT" (at 1.905 8.89 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "ANT_IFA_915MHz_LSM110A" (at 1.905 7.112 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "{G.FP_LIB_NICKNAME}:ANT_IFA_915MHz_LSM110A" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "Datasheet" "Hardware/KiCad/docs/geometria-antena.md" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "Description" "Antena IFA ranurada integrada en PCB para LSM110A (902-928 MHz). El cobre es un solo poligono: el pin FEED y el pin GND estan unidos por el stub de cortocircuito, y el footprint lo declara como net tie." (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "ki_keywords" "antenna IFA PCB 915MHz Sigfox LSM110A net-tie" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "ki_fp_filters" "ANT_IFA*" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (symbol "ANT_IFA_915MHz_LSM110A_0_1"
      (polyline
        (pts
          (xy -3.81 5.08) (xy 3.81 5.08) (xy 3.81 3.81) (xy -3.81 3.81)
          (xy -3.81 2.54) (xy 3.81 2.54)
        )
        (stroke (width 0.254) (type default)) (fill (type none))
      )
      (polyline
        (pts (xy 0 2.54) (xy 0 0))
        (stroke (width 0.254) (type default)) (fill (type none))
      )
      (polyline
        (pts (xy 3.81 2.54) (xy 3.81 0))
        (stroke (width 0.254) (type default)) (fill (type none))
      )
      (text "FEED" (at -0.635 1.27 0)
        (effects (font (size 0.9 0.9)) (justify right))
      )
      (text "GND" (at 4.445 1.27 0)
        (effects (font (size 0.9 0.9)) (justify left))
      )
    )
    (symbol "ANT_IFA_915MHz_LSM110A_1_1"
      (pin passive line (at 0 -2.54 90) (length 2.54)
        (name "FEED" (effects (font (size 1.0 1.0))))
        (number "1" (effects (font (size 1.0 1.0))))
      )
      (pin passive line (at 3.81 -2.54 90) (length 2.54)
        (name "GND" (effects (font (size 1.0 1.0))))
        (number "2" (effects (font (size 1.0 1.0))))
      )
    )
  )
)
"""


def main() -> None:
    out = (Path(sys.argv[1]) if len(sys.argv) > 1
           else G.lib_dir(__file__) / G.SYM_LIB_FILE)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(LIB, encoding="utf-8")
    print(f"escrito: {out}  ({len(LIB)} bytes)")


if __name__ == "__main__":
    main()
