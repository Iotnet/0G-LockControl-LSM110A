#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
==========================================================================
Generación de diagramas — 0G LockControl (módulo LSM110A / STM32WL55)
==========================================================================
Produce 5 figuras PNG en ./figs/ :
  1. casos_uso.png   -> Diagrama de casos de uso (UML)
  2. estados_fsm.png -> Máquina de estados (FSM) del firmware
  3. secuencia.png   -> Diagrama de secuencia (escenario "vandalismo")
  4. karnaugh.png    -> Mapas de Karnaugh de la lógica de clasificación
  5. cronograma.png  -> Cronograma temporal (ventana 10 s + cooldown 60 s)

Requisitos: matplotlib, numpy.
Autor: José Francisco Díaz — I+D, 0G IoT Solutions.
==========================================================================
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import (FancyBboxPatch, FancyArrowPatch, Circle,
                                Rectangle, Ellipse)
import numpy as np

# --------------------------------------------------------------------------
# Paleta corporativa 0G (índigo / púrpura + acento dorado)
# --------------------------------------------------------------------------
INDIGO = "#3B2E7E"
PURPLE = "#6A4C9C"
GOLD   = "#C9A227"
LIGHT  = "#F4F1FA"
MID    = "#D9D2EC"
DARK   = "#1E1B2E"
WHITE  = "#FFFFFF"
RED    = "#B23A48"
GREEN  = "#2E7D5B"
GRAY   = "#8A8598"

plt.rcParams.update({
    "font.family": "DejaVu Sans",
    "font.size": 10,
    "figure.dpi": 160,
})

FIGDIR = os.path.join(os.path.dirname(__file__), "figs")
os.makedirs(FIGDIR, exist_ok=True)


# ==========================================================================
# Helpers de dibujo
# ==========================================================================
def box(ax, x, y, w, h, text, fc=WHITE, ec=INDIGO, tc=DARK, fs=10, lw=2.0,
        bold=True, rounding=0.12):
    """Caja redondeada centrada en (x,y)."""
    p = FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                       boxstyle=f"round,pad=0.02,rounding_size={rounding}",
                       fc=fc, ec=ec, lw=lw, mutation_aspect=1)
    ax.add_patch(p)
    ax.text(x, y, text, ha="center", va="center", fontsize=fs, color=tc,
            weight="bold" if bold else "normal", zorder=5)
    return p


def arrow(ax, p0, p1, label=None, color=INDIGO, rad=0.0, ls="-", lw=1.8,
          fs=8, dx=0.0, dy=0.18, tc=DARK, mscale=14):
    a = FancyArrowPatch(p0, p1, arrowstyle="-|>", mutation_scale=mscale,
                        color=color, lw=lw, ls=ls,
                        connectionstyle=f"arc3,rad={rad}",
                        shrinkA=6, shrinkB=6, zorder=3)
    ax.add_patch(a)
    if label:
        mx = (p0[0] + p1[0]) / 2 + dx
        my = (p0[1] + p1[1]) / 2 + dy
        ax.text(mx, my, label, ha="center", va="center", fontsize=fs, color=tc,
                bbox=dict(boxstyle="round,pad=0.22", fc=WHITE, ec="none",
                          alpha=0.92), zorder=6)


def actor(ax, x, y, label, scale=1.0, color=INDIGO):
    """Figura de palo (actor UML)."""
    r = 0.16 * scale
    ax.add_patch(Circle((x, y), r, fill=False, ec=color, lw=2.2, zorder=4))
    ax.plot([x, x], [y - r, y - 0.95 * scale], color=color, lw=2.2, zorder=4)
    ax.plot([x - 0.34 * scale, x + 0.34 * scale],
            [y - 0.48 * scale, y - 0.48 * scale], color=color, lw=2.2, zorder=4)
    ax.plot([x, x - 0.30 * scale], [y - 0.95 * scale, y - 1.5 * scale],
            color=color, lw=2.2, zorder=4)
    ax.plot([x, x + 0.30 * scale], [y - 0.95 * scale, y - 1.5 * scale],
            color=color, lw=2.2, zorder=4)
    ax.text(x, y - 1.72 * scale, label, ha="center", va="top", fontsize=10,
            color=DARK, weight="bold")


def title_bar(ax, txt, xlim, y):
    """Barra de título con estilo 0G."""
    ax.text(xlim[0] + 0.15, y, txt, ha="left", va="center",
            fontsize=14, weight="bold", color=INDIGO)


# ==========================================================================
# 1) DIAGRAMA DE CASOS DE USO
# ==========================================================================
def fig_casos_uso():
    fig, ax = plt.subplots(figsize=(12, 7.2))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 7.2)
    ax.axis("off")

    title_bar(ax, "0G LockControl — Casos de uso", (0, 12), 6.9)

    # Frontera del sistema
    ax.add_patch(FancyBboxPatch((3.0, 0.6), 6.0, 5.7,
                                boxstyle="round,pad=0.02,rounding_size=0.1",
                                fc=LIGHT, ec=PURPLE, lw=2.0))
    ax.text(6.0, 6.05, "Sistema  0G LockControl", ha="center", va="center",
            fontsize=11, weight="bold", color=PURPLE)

    # Casos de uso (elipses)
    uc = {
        "Armar sistema\n(al salir)":       (4.6, 5.3),
        "Desarmar sistema\n(al volver)":   (4.6, 4.3),
        "Detectar\napertura":              (7.4, 5.35),
        "Detectar\ncierre":                (7.4, 4.4),
        "Detectar\nvandalismo":            (7.4, 3.45),
        "Enviar mensaje\nSigfox":          (6.0, 2.15),
        "Enviar\nheartbeat (24 h)":        (4.6, 3.3),
        "Reportar batería\ny temperatura": (7.5, 2.35),
    }
    ep = {}
    for txt, (x, y) in uc.items():
        w, h = 1.9, 0.95
        e = Ellipse((x, y), w, h, fc=WHITE, ec=INDIGO, lw=1.8, zorder=3)
        ax.add_patch(e)
        ax.text(x, y, txt, ha="center", va="center", fontsize=8, color=DARK,
                zorder=4)
        ep[txt] = (x, y, w, h)

    # Actores
    actor(ax, 1.4, 4.7, "Usuario")
    actor(ax, 1.4, 2.2, "Intruso", color=RED)
    actor(ax, 10.7, 4.5, "Backend\nSigfox", color=GREEN)

    # Asociaciones usuario
    for t in ["Armar sistema\n(al salir)", "Desarmar sistema\n(al volver)"]:
        x, y, w, h = ep[t]
        ax.plot([1.7, x - w / 2], [4.55, y], color=INDIGO, lw=1.4, zorder=1)

    # Asociaciones intruso (dispara detecciones)
    for t in ["Detectar\napertura", "Detectar\nvandalismo"]:
        x, y, w, h = ep[t]
        ax.plot([1.75, x - w / 2], [2.05, y], color=RED, lw=1.4, ls="--",
                zorder=1)

    # Backend recibe
    for t in ["Enviar mensaje\nSigfox", "Reportar batería\ny temperatura"]:
        x, y, w, h = ep[t]
        ax.plot([10.4, x + w / 2], [4.35, y], color=GREEN, lw=1.4, zorder=1)

    # <<include>>: detecciones incluyen "Enviar mensaje Sigfox"
    xm, ym, wm, hm = ep["Enviar mensaje\nSigfox"]
    for t in ["Detectar\napertura", "Detectar\ncierre", "Detectar\nvandalismo",
              "Enviar\nheartbeat (24 h)"]:
        x, y, w, h = ep[t]
        arrow(ax, (x, y - h / 2), (xm, ym + hm / 2), color=GOLD, lw=1.3,
              rad=0.05, mscale=11)
    ax.text(6.9, 3.05, "«include»", fontsize=7.5, color=GOLD, style="italic",
            rotation=-25)

    # Reportar batería es <<include>> del envío
    ax.text(0.15, 0.35,
            "Trazos punteados = disparo por evento externo (intruso).  "
            "Flechas doradas «include» = todo evento culmina en un envío Sigfox.",
            fontsize=8, color=GRAY)

    fig.tight_layout()
    out = os.path.join(FIGDIR, "casos_uso.png")
    fig.savefig(out, bbox_inches="tight", facecolor=WHITE)
    plt.close(fig)
    print("OK", out)


# ==========================================================================
# 2) MÁQUINA DE ESTADOS (FSM)
# ==========================================================================
def fig_estados():
    fig, ax = plt.subplots(figsize=(13.5, 7.4))
    ax.set_xlim(0, 13.5)
    ax.set_ylim(0, 7.4)
    ax.axis("off")

    title_bar(ax, "0G LockControl — Máquina de estados (FSM)", (0, 13.5), 7.1)

    # Estados
    S = {
        "DESARMADO":   (2.2, 5.5),
        "ARMADO":      (2.2, 2.6),
        "OBSERVANDO":  (6.2, 2.6),
        "CLASIFICAR":  (9.4, 2.6),
        "TRANSMITIR":  (12.0, 2.6),
    }
    sub = {
        "DESARMADO":  "sensores ignorados\n(solo heartbeat)",
        "ARMADO":     "Stop2 ~3 µA\ndoor closed, wait",
        "OBSERVANDO": "ventana T_OBS = 10 s\ncuenta N, mide θ_max",
        "CLASIFICAR": "aplica K-map\n(APERTURA/CIERRE/\nVANDALISMO)",
        "TRANSMITIR": "TX Sigfox\n~50 mA × 2 s",
    }
    colors = {
        "DESARMADO": MID, "ARMADO": LIGHT, "OBSERVANDO": "#EDE4C8",
        "CLASIFICAR": LIGHT, "TRANSMITIR": "#F3D9DC",
    }
    w, h = 2.7, 1.35
    for name, (x, y) in S.items():
        box(ax, x, y, w, h, name, fc=colors[name], ec=INDIGO, fs=11)
        ax.text(x, y - 0.42, sub[name], ha="center", va="center", fontsize=7.2,
                color=DARK, style="italic", zorder=5)

    # Estado inicial
    ax.add_patch(Circle((0.55, 5.5), 0.12, fc=DARK, ec=DARK, zorder=5))
    arrow(ax, (0.67, 5.5), (2.2 - w / 2, 5.5), color=DARK, lw=1.8)

    # DESARMADO -> ARMADO  (botón: armar)
    arrow(ax, (1.9, 5.5 - h / 2), (1.9, 2.6 + h / 2),
          label="botón: armar\n(captura accel_ref)", color=INDIGO,
          rad=0.0, dx=-1.35, dy=0.0, fs=7.6)
    # ARMADO -> DESARMADO (botón: desarmar)
    arrow(ax, (2.6, 2.6 + h / 2), (2.6, 5.5 - h / 2),
          label="botón: desarmar", color=GRAY, rad=0.0, dx=1.15, dy=0.0, fs=7.6)

    # ARMADO -> OBSERVANDO (flanco apertura)
    arrow(ax, (2.2 + w / 2, 2.6), (6.2 - w / 2, 2.6),
          label="flanco apertura Hall\n(t0 = RTC, N = 1)", color=INDIGO,
          dy=0.55, fs=7.6)

    # OBSERVANDO self-loop (más flancos)
    ax.add_patch(FancyArrowPatch((6.2 - 0.5, 2.6 + h / 2),
                                 (6.2 + 0.5, 2.6 + h / 2),
                                 arrowstyle="-|>", mutation_scale=13,
                                 color=PURPLE, lw=1.7,
                                 connectionstyle="arc3,rad=-1.6", zorder=3))
    ax.text(6.2, 2.6 + h / 2 + 1.0, "flanco Hall dentro de ventana\n"
            "N++,  θ_max = max(θ_max, θ),  actualiza estado puerta",
            ha="center", va="center", fontsize=7.2, color=PURPLE)

    # OBSERVANDO -> CLASIFICAR (T_OBS agota)
    arrow(ax, (6.2 + w / 2, 2.6), (9.4 - w / 2, 2.6),
          label="T_OBS agota", color=INDIGO, dy=0.35, fs=7.8)

    # CLASIFICAR -> TRANSMITIR
    arrow(ax, (9.4 + w / 2, 2.6), (12.0 - w / 2, 2.6),
          label="arma payload", color=INDIGO, dy=0.35, fs=7.8)

    # TRANSMITIR -> ARMADO (cooldown 60s) curva abajo
    a = FancyArrowPatch((12.0, 2.6 - h / 2), (2.2 + w / 2 - 0.1, 2.6 - h / 2 - 0.05),
                        arrowstyle="-|>", mutation_scale=14, color=GREEN, lw=1.9,
                        connectionstyle="arc3,rad=0.28", shrinkA=6, shrinkB=6,
                        zorder=2)
    ax.add_patch(a)
    ax.text(7.0, 0.55, "cooldown 60 s  →  re-arma (Stop2)",
            ha="center", va="center", fontsize=8.2, color=GREEN, weight="bold")

    # Heartbeat: ARMADO/DESARMADO -> TRANSMITIR (dashed, arriba)
    a2 = FancyArrowPatch((2.2 + w / 2, 5.5), (12.0, 2.6 + h / 2 + 0.05),
                         arrowstyle="-|>", mutation_scale=13, color=GOLD, lw=1.7,
                         ls="--", connectionstyle="arc3,rad=-0.30",
                         shrinkA=6, shrinkB=6, zorder=2)
    ax.add_patch(a2)
    ax.text(7.2, 6.15, "timer heartbeat 24 h  →  TX (tipo = heartbeat)",
            ha="center", va="center", fontsize=8.2, color=GOLD, weight="bold")

    fig.tight_layout()
    out = os.path.join(FIGDIR, "estados_fsm.png")
    fig.savefig(out, bbox_inches="tight", facecolor=WHITE)
    plt.close(fig)
    print("OK", out)


# ==========================================================================
# 3) DIAGRAMA DE SECUENCIA (escenario vandalismo)
# ==========================================================================
def fig_secuencia():
    fig, ax = plt.subplots(figsize=(14.5, 9.2))
    ax.set_xlim(0, 15.5)
    ax.set_ylim(0, 9.6)
    ax.axis("off")

    title_bar(ax, "0G LockControl — Secuencia (escenario: forcejeo / vandalismo)",
              (0, 15.5), 9.35)

    parts = [
        ("Usuario",           1.2,  INDIGO),
        ("Botón",             3.3,  INDIGO),
        ("Sensor Hall\n(DRV5032)", 5.5, INDIGO),
        ("Acelerómetro\n(LIS2DW12)", 7.7, INDIGO),
        ("MCU / FSM\n(STM32WL)", 9.9, PURPLE),
        ("RTC",              12.0, INDIGO),
        ("Sigfox\n+ Backend", 14.2, GREEN),
    ]
    xpos = {}
    top = 8.75
    bot = 0.9
    for name, x, c in parts:
        box(ax, x, top, 1.75, 0.7, name, fc=LIGHT, ec=c, fs=8.3)
        ax.plot([x, x], [top - 0.4, bot], color=GRAY, lw=1.1, ls=(0, (4, 3)),
                zorder=1)
        xpos[name.split("\n")[0]] = x

    def msg(y, a, b, txt, color=INDIGO, ls="-", fs=8):
        arrow(ax, (xpos[a], y), (xpos[b], y), color=color, ls=ls, lw=1.6,
              mscale=12)
        midx = (xpos[a] + xpos[b]) / 2
        ax.text(midx, y + 0.16, txt, ha="center", va="bottom", fontsize=fs,
                color=DARK, zorder=6,
                bbox=dict(boxstyle="round,pad=0.18", fc=WHITE, ec="none",
                          alpha=0.9))

    def selfmsg(y, a, txt, color=PURPLE, fs=8):
        x = xpos[a]
        ax.add_patch(FancyArrowPatch((x, y + 0.05), (x, y - 0.35),
                     arrowstyle="-|>", mutation_scale=11, color=color, lw=1.5,
                     connectionstyle="arc3,rad=-2.2", zorder=3))
        ax.text(x + 0.25, y - 0.15, txt, ha="left", va="center", fontsize=fs,
                color=color, zorder=6,
                bbox=dict(boxstyle="round,pad=0.18", fc=WHITE, ec="none",
                          alpha=0.9))

    y = 8.15
    dy = 0.62
    msg(y, "Usuario", "Botón", "presiona al salir");                       y -= dy
    msg(y, "Botón", "MCU / FSM", "EXTI botón → armar", color=INDIGO);      y -= dy
    msg(y, "MCU / FSM", "Acelerómetro", "captura accel_ref (pos. inicial)");y -= dy
    selfmsg(y, "MCU / FSM", "estado = ARMADO · entra Stop2 (~3 µA)");      y -= dy

    # separador "forcejeo"
    ax.add_patch(Rectangle((0.5, y - 0.18), 15.0, 0.34, fc="#F3D9DC",
                           ec=RED, lw=0.8, zorder=1, alpha=0.8))
    ax.text(0.7, y, "  ‣  intruso forcejea la puerta (cerrada con llave)  ‣", ha="left",
            va="center", fontsize=8.5, color=RED, weight="bold", zorder=5)
    y -= dy

    msg(y, "Sensor Hall", "MCU / FSM", "flanco apertura (WAKE)", color=RED,
        ls="--");                                                          y -= dy
    msg(y, "MCU / FSM", "RTC", "t_apertura = now · arranca T_OBS (10 s)"); y -= dy
    msg(y, "Sensor Hall", "MCU / FSM", "flancos ×N  (N++)", color=RED,
        ls="--");                                                          y -= dy
    msg(y, "MCU / FSM", "Acelerómetro", "lee θ · θ_max = max(θ_max, θ)");  y -= dy
    selfmsg(y, "MCU / FSM",
            "T_OBS agota → N>5 ∧ θ_max<umbral  ⇒  VANDALISMO", color=RED); y -= dy
    msg(y, "MCU / FSM", "Sigfox", "TX payload 12 B (~50 mA × 2 s)",
        color=GREEN);                                                      y -= dy
    ax.text(xpos["Sigfox"], y + 0.02, "trama + seq + timestamp\n(los pone la red)",
            ha="center", va="top", fontsize=7.3, color=GREEN, style="italic")
    y -= dy * 0.7
    selfmsg(y, "MCU / FSM", "cooldown 60 s → ARMADO / Stop2", color=GREEN)

    ax.text(0.15, 0.35,
            "Flechas rojas punteadas = eventos asíncronos que despiertan al MCU "
            "(EXTI).  La hora la aporta el backend Sigfox; el firmware sólo mide "
            "duración con el RTC.", fontsize=8, color=GRAY)

    fig.tight_layout()
    out = os.path.join(FIGDIR, "secuencia.png")
    fig.savefig(out, bbox_inches="tight", facecolor=WHITE)
    plt.close(fig)
    print("OK", out)


# ==========================================================================
# 4) MAPAS DE KARNAUGH
# ==========================================================================
def _kmap(ax, title, func, eq, eq_color=INDIGO):
    """Dibuja un K-map 2x4 (C filas, MP columnas en código Gray)."""
    cols = [(0, 0), (0, 1), (1, 1), (1, 0)]      # (M,P) Gray
    col_lbl = ["00", "01", "11", "10"]
    ax.set_xlim(-1.4, 4.4)
    ax.set_ylim(-1.9, 2.7)
    ax.axis("off")
    ax.text(1.5, 2.45, title, ha="center", va="center", fontsize=11,
            weight="bold", color=INDIGO)
    # etiquetas ejes
    ax.text(-0.95, 1.9, "C\\MP", ha="center", va="center", fontsize=9,
            color=DARK, weight="bold")
    for j, cl in enumerate(col_lbl):
        ax.text(j + 0.5, 1.75, cl, ha="center", va="center", fontsize=9,
                color=DARK, weight="bold")
    for i, C in enumerate([0, 1]):
        yy = 1.0 - i  # fila
        ax.text(-0.55, yy + 0.5, str(C), ha="center", va="center", fontsize=9,
                color=DARK, weight="bold")
        for j, (M, P) in enumerate(cols):
            v = func(C, M, P)
            fc = GOLD if v else LIGHT
            tc = WHITE if v else GRAY
            ax.add_patch(Rectangle((j, yy), 1, 1, fc=fc, ec=INDIGO, lw=1.3))
            ax.text(j + 0.5, yy + 0.5, str(v), ha="center", va="center",
                    fontsize=13, color=tc, weight="bold")
    ax.text(1.5, -1.35, eq, ha="center", va="center", fontsize=11,
            color=eq_color, weight="bold",
            bbox=dict(boxstyle="round,pad=0.3", fc=WHITE, ec=eq_color, lw=1.2))


def fig_karnaugh():
    # Variables:  C = (N>5),  M = movimiento significativo,  P = puerta abierta
    f_van = lambda C, M, P: 1 if (C == 1 and M == 0) else 0
    f_ape = lambda C, M, P: 1 if (P == 1 and not (C == 1 and M == 0)) else 0
    f_cer = lambda C, M, P: 1 if (P == 0 and not (C == 1 and M == 0)) else 0

    fig, axes = plt.subplots(1, 3, figsize=(14, 4.6))
    fig.suptitle("0G LockControl — Clasificación del evento (mapas de Karnaugh)",
                 fontsize=14, weight="bold", color=INDIGO, y=1.02, x=0.5,
                 ha="center")
    _kmap(axes[0], "VANDALISMO",
          f_van, r"$V = C \cdot \overline{M}$", RED)
    _kmap(axes[1], "APERTURA",
          f_ape, r"$A = P\,(\overline{C} + M)$", INDIGO)
    _kmap(axes[2], "CIERRE",
          f_cer, r"$Z = \overline{P}\,(\overline{C} + M)$", GREEN)

    # nota
    fig.text(0.5, -0.04,
             "C = (N > 5 aperturas)   ·   M = (θ_max ≥ umbral de movimiento)   ·"
             "   P = (puerta abierta al cierre de la ventana)."
             "   Las 3 salidas son mutuamente excluyentes y cubren los 8 casos.",
             ha="center", fontsize=9, color=GRAY)
    fig.tight_layout(rect=[0, 0.02, 1, 0.98])
    out = os.path.join(FIGDIR, "karnaugh.png")
    fig.savefig(out, bbox_inches="tight", facecolor=WHITE)
    plt.close(fig)
    print("OK", out)


# ==========================================================================
# 5) CRONOGRAMA TEMPORAL
# ==========================================================================
def fig_cronograma():
    fig, axes = plt.subplots(3, 1, figsize=(13.5, 7.6), sharex=True,
                             gridspec_kw={"height_ratios": [1, 1, 1.1]})
    fig.suptitle("0G LockControl — Cronograma: ventana de observación (10 s) "
                 "+ cooldown (60 s)", fontsize=13.5, weight="bold",
                 color=INDIGO, y=0.98)

    t_end = 78
    t = np.linspace(0, t_end, 4000)

    # --- Eventos Hall (burst de forcejeo: 6 aperturas en < 10 s) ---
    edges = [1.0, 2.3, 3.6, 5.0, 6.4, 8.1]        # flancos de apertura
    widths = [0.5, 0.4, 0.45, 0.5, 0.4, 0.6]
    hall = np.zeros_like(t)
    for e, wdt in zip(edges, widths):
        hall[(t >= e) & (t < e + wdt)] = 1.0

    axH = axes[0]
    axH.plot(t, hall, color=INDIGO, lw=1.8, drawstyle="steps-post")
    axH.fill_between(t, 0, hall, step="post", color=INDIGO, alpha=0.12)
    axH.set_ylim(-0.25, 1.4)
    axH.set_yticks([0, 1])
    axH.set_yticklabels(["cerrada", "abierta"])
    axH.set_ylabel("Sensor Hall", color=INDIGO, weight="bold")
    axH.text(4.5, 1.2, "N = 6 aperturas en la ventana  (N > 5)", color=RED,
             fontsize=9, weight="bold", ha="center")
    for e in edges:
        axH.axvline(e, color=RED, lw=0.7, ls=":", alpha=0.6)

    # --- θ (desplazamiento acelerómetro) ---
    axT = axes[1]
    theta = np.zeros_like(t)
    for e, wdt in zip(edges, widths):
        m = (t >= e) & (t < e + wdt + 0.3)
        theta[m] += 9 * np.exp(-((t[m] - (e + wdt / 2)) ** 2) / 0.05)
    theta = np.clip(theta, 0, 14)
    umbral = 20  # umbral de "movimiento significativo" (grados equiv.)
    axT.plot(t, theta, color=PURPLE, lw=1.8)
    axT.axhline(umbral, color=GOLD, lw=1.6, ls="--")
    axT.text(t_end - 1, umbral + 1.2, "umbral θ (movimiento significativo)",
             color=GOLD, fontsize=8.5, ha="right", weight="bold")
    axT.text(9.2, 12.8, "θ_max ≈ 9°  <  umbral  ⇒  la puerta NO se abrió de "
             "verdad", color=PURPLE, fontsize=8.7, ha="left")
    axT.set_ylim(0, 30)
    axT.set_ylabel("θ acelerómetro\n(desde accel_ref)", color=PURPLE,
                   weight="bold")

    # --- Corriente (consumo) ---
    axI = axes[2]
    cur = np.full_like(t, 0.003)                  # Stop2 ~3 µA (mostrado en mA)
    # pulsos pequeños al despertar por cada flanco (Run ~pocos ms) -> pico chico
    for e in edges:
        m = (t >= e) & (t < e + 0.15)
        cur[m] = 3.0
    # ventana termina en 10 s -> clasifica -> TX 50 mA x 2 s
    tx0, tx1 = 10.0, 12.0
    cur[(t >= tx0) & (t < tx1)] = 50.0
    axI.plot(t, cur, color=DARK, lw=1.6)
    axI.fill_between(t, 0.003, cur, color=DARK, alpha=0.10)
    axI.set_yscale("log")
    axI.set_ylim(0.001, 120)
    axI.set_ylabel("Corriente (mA)\n[log]", color=DARK, weight="bold")
    axI.set_xlabel("tiempo (s)")
    axI.text((tx0 + tx1) / 2, 68, "TX Sigfox  ~50 mA × 2 s\n(dominante en "
             "energía)", color=RED, fontsize=8.7, ha="center", weight="bold")
    axI.text(40, 0.012, "Stop2  ~3 µA  (RTC corriendo — medir t_abierto es "
             "prácticamente gratis)", color=GREEN, fontsize=8.7, ha="center",
             weight="bold")

    # Bandas de ventana y cooldown en los 3 ejes
    for ax_ in axes:
        ax_.axvspan(1.0, 10.0, color=GOLD, alpha=0.10)
        ax_.axvspan(12.0, 72.0, color=GREEN, alpha=0.07)
    axes[0].axvspan(1.0, 10.0, color=GOLD, alpha=0.0)  # no-op keep order
    axH.text(5.5, -0.18, "◄ ventana T_OBS = 10 s ►", color=GOLD, fontsize=8.7,
             ha="center", weight="bold")
    axI.text(42, 0.0016, "◄————— cooldown 60 s (anti-flood Sigfox) —————►",
             color=GREEN, fontsize=8.7, ha="center", weight="bold")
    axI.axvline(10, color=GOLD, lw=1.2, ls="--")
    axI.text(10, 0.0011, "clasifica", color=GOLD, fontsize=8, ha="center")

    for ax_ in axes:
        ax_.grid(True, axis="x", ls=":", alpha=0.35)
        ax_.set_xlim(0, t_end)

    fig.tight_layout(rect=[0, 0, 1, 0.96])
    out = os.path.join(FIGDIR, "cronograma.png")
    fig.savefig(out, bbox_inches="tight", facecolor=WHITE)
    plt.close(fig)
    print("OK", out)


# ==========================================================================
if __name__ == "__main__":
    fig_casos_uso()
    fig_estados()
    fig_secuencia()
    fig_karnaugh()
    fig_cronograma()
    print("\nTodas las figuras generadas en:", FIGDIR)
