# Especificaciones de diseño PCB — 0G LockControl v1

Fuentes: User Manual FCC 5937666 · datasheet LSM110A R03 · decisiones DT-001 a DT-011

## Stackup
- 2 capas, FR4 1.6 mm, 1 oz cobre, εr = 4.3 (igual a referencia SJI)
- Top: componentes + señales · Bottom: plano GND sólido SIN cortes bajo módulo ni antena

## Reglas RF (obligatorias — certificación)
- Línea RF: CPWG 50 Ω, 1.0 mm ancho / 0.15 mm clearance a GND coplanar
- Matching en pin 33: L101 0 Ω (serie) + C101 2.2 pF (shunt) + C102 DNI — footprints 0402
- Antena: patrón EVB_LSM ANT de SJI, copia exacta del Gerber (pendiente NDA GREATECH)
- Keepout total bajo/alrededor de antena: sin cobre, sin componentes, sin plano
- Via stitching perimetral en zona RF (paso ≈ 8 mm ≈ λ/20 @ 915 MHz)
- Línea RF: mínima longitud, sin ángulos de 90° (usar 45° o curvas)

## Dimensiones
- Referencia SJI: 50×80 mm — la antena serpentín + keepout dominan un extremo
- El ancho de la antena puede forzar ≥ 50 mm en un eje; confirmar al recibir Gerber
- Restricción de producto: caja compacta (objetivo < 5 cm en el eje visible) — revisar contra antena real

## Placement (orden de prioridad)
1. Antena en borde superior, keepout respetado, lejos de batería y metal
2. LSM110A adyacente al matching, línea RF corta y recta
3. CR2450 + 470 µF en extremo opuesto a la antena (la pila es plano metálico → detuning)
4. LIS2DW12 cerca del módulo (I²C corto), fuera de la zona RF
5. Reed/DRV5032 en el borde que da al imán (definir orientación de montaje)
6. SWD accesible con el dispositivo ensamblado

## Reglas generales
- Desacoplo 100 nF a < 2 mm de cada pin VDD; 10 µF bulk en la entrada del rail
- 470 µF lo más cerca del lazo batería→módulo (corriente de TX)
- GPIOs sin usar: no rutear (se configuran analog en FW)
- Serigrafía: versión, polaridad batería, pinout SWD, "Contains FCC ID: 2AS8LLSM110A"
- DRC JLCPCB 2 capas: track/space ≥ 0.127 mm, via ≥ 0.3/0.6 mm

## Checklist pre-gerber
- [ ] Impedancia CPWG verificada (1.0 / 0.15 / 1.6 mm / εr 4.3 ≈ 50 Ω)
- [ ] Antena importada del Gerber oficial SJI (NO redibujada a mano)
- [ ] GND continuo bajo módulo + stitching RF
- [ ] GATE 2 cerrado → confirma CR2450 + 470 µF (o pivot CR2477 / 1000 µF)
- [ ] Revisión cruzada con Yahir (pines vs firmware)
- [ ] DRC = 0 errores
