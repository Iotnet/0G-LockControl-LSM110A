# Especificaciones de diseño PCB — 0G LockControl v1

Fuentes: User Manual FCC 5937666 · datasheet LSM110A R03 · decisiones DT-001 a DT-011

## Stackup
- 2 capas, FR4 1.6 mm, 1 oz cobre, εr = 4.3 (igual a referencia SJI)
- Top: componentes + señales · Bottom: plano GND sólido SIN cortes bajo módulo ni antena

## Reglas RF (obligatorias — certificación)
- Línea RF: CPWG 50 Ω, 1.0 mm ancho / 0.15 mm clearance a GND coplanar
- Matching en pin 33: L101 0 Ω (serie) + C101 2.2 pF (shunt) + C102 DNI — footprints 0402
- Antena: patrón `EVB_LSM ANT` de SJI. Réplica verificada contra el arte de producción
  (desviaciones < 0.10 mm); pendiente el visto bueno documental de SJI/GREATECH
- Keepout total bajo/alrededor de antena: sin cobre, sin componentes, sin plano, **en ambas capas**,
  desde el borde superior de la placa hasta 20.03 mm — el plano de tierra arranca exactamente ahí
- Via stitching perimetral en zona RF (paso ≈ 8 mm ≈ λ/20 @ 915 MHz)
- Línea RF: mínima longitud, sin ángulos de 90° (usar 45° o curvas)

## Dimensiones
- Referencia SJI: 50×80 mm — la antena + su keepout dominan un extremo
- La antena **no es un serpentín**: es una placa ranurada de 39.50 × 13.00 mm (dos ranuras de
  0.50 mm por bordes opuestos) dentro de una pestaña de 50.00 × 20.03 mm, verificada contra el
  arte de producción de SJI — ver `Hardware/v0-replica-sji/00-fuente-de-verdad/antena-cotas.md` §2.3/§2.5
- El ancho de la antena **fuerza 50 mm** en un eje (medido, ya no «puede forzar»): el cobre son
  39.50 mm centrados, con 5.25 mm de margen a cada lado
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
- [ ] Antena: confirmacion por escrito de SJI/GREATECH de que la replica es aceptable
      (el exhibit de FCC exige «must use EVB_LSM ANT», y asigna la verificacion al
      fabricante del producto — pedir el Gerber, o el visto bueno sobre el nuestro)
      → el redibujo parametrico de `Hardware/KiCad/` (`ANT_IFA_915MHz_LSM110A`) ya esta
        cotejado pixel a pixel contra DOS fuentes independientes de SJI: el plano
        acotado del UM §1.5 y el ARTE DE PRODUCCION del exhibit de FCC pag. 5
        (300 dpi, 0.064 mm/px). Todas las desviaciones < 0.10 mm. Ver
        `Hardware/v0-replica-sji/00-fuente-de-verdad/antena-cotas.md` §2.5.
        Lo que falta NO es geometria, es la aprobacion documental.
- [ ] GND continuo bajo módulo + stitching RF
- [ ] GATE 2 cerrado → confirma CR2450 + 470 µF (o pivot CR2477 / 1000 µF)
- [ ] Revisión cruzada con Yahir (pines vs firmware)
- [ ] DRC = 0 errores
