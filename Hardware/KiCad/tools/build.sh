#!/usr/bin/env bash
#
# Regenera y verifica todo el diseno de la antena desde antenna_geometry.py.
#
#   ./build.sh
#
# Requiere KiCad 7 o superior con los bindings de Python (paquete kicad en
# Debian/Ubuntu, o el pcbnew que trae la instalacion oficial).
#
# Falla con codigo != 0 si cualquier comprobacion no pasa, asi que sirve tal
# cual en CI.

set -euo pipefail

cd "$(dirname "$0")"

LIB=../libraries
BOARD=../antenna-test-board
EXPORT=../export

echo "== 1/6  comprobando la cadena de cotas =========================="
python3 antenna_geometry.py

echo
echo "== 2/6  generando footprints ==================================="
python3 gen_footprints.py "$LIB/0G_Antenna.pretty"

echo
echo "== 3/6  generando simbolo ======================================"
python3 gen_symbol.py "$LIB/0G_Antenna.kicad_sym"

echo
echo "== 4/6  generando placa de prueba =============================="
python3 gen_test_board.py "$BOARD"

echo
echo "== 5/6  verificando footprints con el motor de KiCad ==========="
python3 verify.py "$LIB/0G_Antenna.pretty"

echo
echo "== 6/6  verificando la placa (DRC + cotas del cobre real) ======"
python3 verify_board.py "$BOARD/antenna-test-board.kicad_pcb"

echo
echo "== extras: render y exportaciones =============================="
mkdir -p "$EXPORT"
python3 render_preview.py "$EXPORT/antenna-geometry-preview.svg"

kicad-cli pcb export svg \
    --layers F.Cu,Edge.Cuts --page-size-mode 2 --exclude-drawing-sheet \
    -o "$EXPORT/test-board-F_Cu.svg" \
    "$BOARD/antenna-test-board.kicad_pcb" >/dev/null
echo "escrito: $EXPORT/test-board-F_Cu.svg"

kicad-cli pcb export svg \
    --layers B.Cu,Edge.Cuts --page-size-mode 2 --exclude-drawing-sheet \
    -o "$EXPORT/test-board-B_Cu.svg" \
    "$BOARD/antenna-test-board.kicad_pcb" >/dev/null
echo "escrito: $EXPORT/test-board-B_Cu.svg"

kicad-cli sym export svg --output "$EXPORT/symbol" \
    "$LIB/0G_Antenna.kicad_sym" >/dev/null
echo "escrito: $EXPORT/symbol/"

# Los SVG exportados llevan la fecha de generacion en su <title>. Se versionan
# en git, asi que se normaliza: si no, cada build los deja modificados sin que
# haya cambiado la geometria.
for f in "$EXPORT"/*.svg "$EXPORT"/symbol/*.svg; do
    [ -f "$f" ] || continue
    sed -i -E 's#(<title>SVG Image created as [^ ]+) date [0-9/: ]*#\1#' "$f"
done
echo "normalizadas las marcas de tiempo de los SVG"

echo
echo "TODO OK - antena regenerada y verificada"
