# Certificación FCC — LSM110A (FCC ID: 2AS8LLSM110A / IC: 25119-LSM110A)

Documentos del expediente FCC. Fuente: fcc.report/FCC-ID/2AS8LLSM110A

## Datos críticos de diseño (User Manual, doc 5937666)

**PCB de referencia SJI:** 50×80 mm · 2 capas · FR4 1.6 mm · εr = 4.3

**Línea RF:** CPWG 50 Ω — ancho 1.0 mm / clearance 0.15 mm (NO es microstrip)

**Matching network (pin 33 RFOUT → antena):** L101 = 0 Ω serie · C101 = 2.2 pF shunt · C102 = DNI

**Antena:** patrón "EVB_LSM ANT" de SJI · monopolo serpentín · 1.9 dBi pico · cuasi-omni.
SOLO se acepta este patrón — otra antena invalida la certificación modular.
El Gerber es CONFIDENCIAL: solicitar a GREATECH (distribuidor) o SJI bajo NDA.

## Requisitos de integración (KDB 996369 D03)
- Separación mínima antena-personas: 20 cm (documentar en manual del producto)
- Etiquetado del producto final: "Contains FCC ID: 2AS8LLSM110A" y "Contains IC: 25119-LSM110A"
- El producto final requiere prueba Part 15 Subpart B (radiadores no intencionales)
- TX máx +21 dBm ±2 dB
