#!/usr/bin/env python3
"""
Render SVG de la geometria de antenna_geometry.py, para comparar contra el plano
"1.5 Antenna Dimension" antes de generar los archivos de KiCad.

Uso:  python3 render_preview.py ../export/antenna-geometry-preview.svg
"""

from __future__ import annotations

import sys
from pathlib import Path

import antenna_geometry as G

SCALE = 18.0  # px por mm
# margenes asimetricos: a la izquierda cabe la pila de cotas, a la derecha
# la cota del stub, arriba las cotas horizontales.
PAD_L, PAD_T, PAD_R, PAD_B = 32.0, 20.0, 46.0, 19.0


def mm(v: float) -> float:
    return v * SCALE


class Svg:
    def __init__(self, x0, y0, x1, y1):
        self.x0, self.y0 = x0, y0
        self.w, self.h = x1 - x0, y1 - y0
        self.parts: list[str] = []

    def _x(self, x):
        return mm(x - self.x0)

    def _y(self, y):
        return mm(y - self.y0)

    def rect(self, r, **kw):
        x0, y0, x1, y1 = r
        self.parts.append(
            f'<rect x="{self._x(x0):.2f}" y="{self._y(y0):.2f}" '
            f'width="{mm(x1 - x0):.2f}" height="{mm(y1 - y0):.2f}" {self._attrs(kw)}/>'
        )

    def rrect(self, r, radius, **kw):
        x0, y0, x1, y1 = r
        self.parts.append(
            f'<rect x="{self._x(x0):.2f}" y="{self._y(y0):.2f}" '
            f'width="{mm(x1 - x0):.2f}" height="{mm(y1 - y0):.2f}" '
            f'rx="{mm(radius):.2f}" {self._attrs(kw)}/>'
        )

    def poly(self, pts, **kw):
        s = " ".join(f"{self._x(x):.2f},{self._y(y):.2f}" for x, y in pts)
        self.parts.append(f'<polygon points="{s}" {self._attrs(kw)}/>')

    def line(self, x0, y0, x1, y1, **kw):
        self.parts.append(
            f'<line x1="{self._x(x0):.2f}" y1="{self._y(y0):.2f}" '
            f'x2="{self._x(x1):.2f}" y2="{self._y(y1):.2f}" {self._attrs(kw)}/>'
        )

    def text(self, x, y, s, size=3.0, anchor="middle", **kw):
        kw.setdefault("fill", "#1f2937")
        kw.setdefault("font-family", "DejaVu Sans, Helvetica, Arial, sans-serif")
        self.parts.append(
            f'<text x="{self._x(x):.2f}" y="{self._y(y):.2f}" '
            f'font-size="{mm(size):.2f}" text-anchor="{anchor}" '
            f'{self._attrs(kw)}>{s}</text>'
        )

    @staticmethod
    def _attrs(kw):
        return " ".join(f'{k}="{v}"' for k, v in kw.items())

    def render(self) -> str:
        return (
            f'<svg xmlns="http://www.w3.org/2000/svg" '
            f'width="{mm(self.w):.0f}" height="{mm(self.h):.0f}" '
            f'viewBox="0 0 {mm(self.w):.2f} {mm(self.h):.2f}">'
            f'<rect width="100%" height="100%" fill="#ffffff"/>'
            + "".join(self.parts)
            + "</svg>"
        )


# ---------------------------------------------------------------- dimensiones

def dim_h(svg, x0, x1, y, label, tick=1.6, off=0.0):
    """Cota horizontal con flechas."""
    st = dict(stroke="#334155", **{"stroke-width": "0.9"})
    svg.line(x0, y, x1, y, **st)
    for x in (x0, x1):
        svg.line(x, y - tick / 2, x, y + tick / 2, **st)
    svg.text((x0 + x1) / 2 + off, y - 1.0, label, size=2.4)


def dim_v(svg, y0, y1, x, label, tick=1.6):
    st = dict(stroke="#334155", **{"stroke-width": "0.9"})
    svg.line(x, y0, x, y1, **st)
    for y in (y0, y1):
        svg.line(x - tick / 2, y, x + tick / 2, y, **st)
    svg.text(x - 0.8, (y0 + y1) / 2 + 0.8, label, size=2.4, anchor="end")


def build(path: Path) -> None:
    board_bottom = G.C101_PAD_Y + 3.5
    x0 = G.BOARD_LEFT - PAD_L
    y0 = G.BOARD_TOP - PAD_T
    x1 = G.BOARD_RIGHT + PAD_R
    y1 = board_bottom + PAD_B

    s = Svg(x0, y0, x1, y1)

    # ---- contorno de la PCB de referencia -------------------------------
    s.rrect(
        (G.BOARD_LEFT, G.BOARD_TOP, G.BOARD_RIGHT, board_bottom),
        2.0,
        fill="none", stroke="#c026d3", **{"stroke-width": "1.2"},
    )

    # ---- plano de tierra -------------------------------------------------
    s.rect(
        (G.BOARD_LEFT, G.Y_GND, G.BOARD_RIGHT, board_bottom),
        fill="#dcfce7", stroke="#86efac", **{"stroke-width": "0.6"},
    )
    s.text(G.BOARD_LEFT + 1.5, G.Y_GND + 7.8, "Plano de tierra (GND)", size=2.2, anchor="start")

    # ---- area de keepout -------------------------------------------------
    s.rect(
        G.KEEPOUT,
        fill="none", stroke="#f59e0b", **{"stroke-width": "0.8", "stroke-dasharray": "3 2"},
    )
    s.text(G.BOARD_LEFT, G.BOARD_TOP - 1.4,
           "keepout F.Cu + B.Cu: sin plano / vias / componentes",
           size=2.2, anchor="start", fill="#b45309")

    # ---- cobre de la antena ---------------------------------------------
    s.poly(G.OUTLINE, fill="#86c56a", stroke="#2f6b1f", **{"stroke-width": "0.5"})

    # ranuras (para que se vean marcadas igual que en el plano)
    for _, r in G.SLOTS:
        s.rect(r, fill="#ffffff", stroke="#2f6b1f", **{"stroke-width": "0.35"})

    # ---- fila de troquelado del EVM (solo referencia) --------------------
    for _, sx0, sx1 in G.SLOTROW_X:
        s.rrect(
            (sx0, G.SLOTROW_Y0, sx1, G.SLOTROW_Y1), G.SLOTROW_H / 2,
            fill="#ffffff", stroke="#c026d3", **{"stroke-width": "0.8"},
        )

    # ---- linea CPWG + C101 ----------------------------------------------
    s.rect((G.FEED_X0, G.Y_GND, G.FEED_X1, G.C101_PAD_Y),
           fill="#86c56a", stroke="#2f6b1f", **{"stroke-width": "0.4"})
    for sgn in (-1, +1):
        gx = G.FEED_X0 - G.CPWG_GAP if sgn < 0 else G.FEED_X1
        s.rect((gx, G.Y_GND, gx + G.CPWG_GAP, G.C101_PAD_Y),
               fill="#ffffff", stroke="none")
    s.rect((G.FEED_X0 - 0.15, G.C101_PAD_Y, G.FEED_X1 + 0.15, G.C101_PAD_Y + 1.0),
           fill="#475569")
    s.text(G.FEED_X1 + 1.6, G.C101_PAD_Y + 0.8, "L101 serie + C101 shunt", size=2.4,
           anchor="start")

    # ---- etiquetas -------------------------------------------------------
    s.text(G.ANT_W / 2, -1.4, "ANTENA - cobre capa TOP (un solo poligono, IFA)",
           size=2.6, fill="#2f6b1f")
    s.text(G.STUB_X1 + 2.0, G.STUB_TOP_Y + 4.6, "stub L a GND", size=2.4, anchor="start",
           fill="#2f6b1f")

    # ---- cotas -----------------------------------------------------------
    dim_h(s, G.BOARD_LEFT, G.BOARD_RIGHT, G.BOARD_TOP - 12.0, f"{G.BOARD_W:.2f}")
    dim_h(s, 0.0, G.ANT_W, G.BOARD_TOP - 8.0, f"{G.ANT_W:.2f}")
    dim_h(s, 0.0, G.SLOT1_LEN, G.BOARD_TOP - 4.0, f"{G.SLOT1_LEN:.2f}")
    dim_h(s, 0.0, G.BRIDGE_L, G.Y_SLOT2_B + 4.0, f"{G.BRIDGE_L:.2f}")
    dim_h(s, G.STUB_X0, G.STUB_X1, G.STUB_TOP_Y - 1.2, f"{G.STUB_RUN:.2f}")

    dim_v(s, G.BOARD_TOP, G.Y_GND, -18.0, f"{G.EDGE_TO_GND:.2f}")
    dim_v(s, G.BOARD_TOP, 0.0, -13.0, f"{G.EDGE_TO_ANT_TOP:.2f}")
    dim_v(s, 0.0, G.Y_SLOT1_T, -8.0, f"{G.ARM_TOP_H:.2f}")
    dim_v(s, G.Y_SLOT1_B, G.Y_BOT, -8.0, "7.50")
    dim_v(s, G.Y_BOT, G.Y_GND, -8.0, f"{G.ANT_TO_GND:.2f}")
    dim_v(s, G.Y_SLOT1_B, G.Y_SLOT2_T, 8.0, f"{G.ARM_MID_H:.2f}")
    dim_v(s, G.Y_SLOT2_B, G.Y_BOT, 26.0, f"{G.ARM_BOT_H:.2f}")
    dim_v(s, 0.0, G.Y_SLOT2_T, G.ANT_W + 8.0, f"{G.RIGHT_EDGE_H:.2f}")
    dim_v(s, G.STUB_TOP_Y, G.Y_GND, G.ANT_W + 25.0, f"{G.STUB_BBOX_H:.2f}")
    dim_v(s, G.Y_GND, G.C101_PAD_Y, G.FEED_X0 - 8.0, f"{G.CPWG_LEN:.2f}")

    # ranuras de 0.50
    s.text(G.SLOT1_LEN + 3.0, G.Y_SLOT1_T - 0.4, f"{G.SLOT_W:.2f}", size=2.2, anchor="start")
    s.text(G.BRIDGE_L + 2.0, G.Y_SLOT2_T - 0.4, f"{G.SLOT_W:.2f}", size=2.2, anchor="start")
    s.text(G.FEED_X1 + 1.0, G.Y_GND + 1.9, f"{G.FEED_W:.2f}", size=2.2, anchor="start")

    # ---- pie -------------------------------------------------------------
    s.text(
        G.BOARD_LEFT, board_bottom + 5.5,
        f"ANT_IFA_915MHz_LSM110A - cotas en mm - cobre {G.copper_area():.2f} mm2",
        size=2.6, anchor="start", fill="#475569",
    )
    s.text(
        G.BOARD_LEFT, board_bottom + 9.8,
        "generado de tools/antenna_geometry.py - 0G LockControl",
        size=2.2, anchor="start", fill="#94a3b8",
    )

    path.write_text(s.render(), encoding="utf-8")
    print(f"escrito: {path}  ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "../export/antenna-geometry-preview.svg")
    out.parent.mkdir(parents=True, exist_ok=True)
    build(out)
