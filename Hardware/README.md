# Hardware

Diseño electrónico del dispositivo 0G LockControl basado en el SoC LSM110A.

## Subcarpetas

### Schematic/
Esquemáticos del circuito. Incluye el sistema mínimo del LSM110A: alimentación (LDO), oscilador, protección ESD, conector de programación (SWD/UART), reed switch, y matching network de la antena.

### PCB/
Diseño de la tarjeta de circuito impreso. **2 capas, FR4 1.6 mm** — no una sola capa: la antena IFA
necesita un plano de tierra sólido y continuo, porque **el plano es parte del radiador**, no solo
retorno de corriente (ver `certificacion-FCC/` y `v0-replica-sji/00-fuente-de-verdad/antena-cotas.md` §4).
La referencia de SJI es de 2 capas y copiar el patrón sobre un plano distinto desafina la antena.
Considera separación entre circuitos digitales y RF, y rutas críticas aisladas según la guía AN5457 de ST.

### BOM/
Lista de materiales con referencias de proveedores (Mouser, DigiKey, LCSC), cantidades, costos unitarios y alternativas.

### Datasheets/
Hojas de datos de los componentes principales: LSM110A, reed switch, regulador LDO, antena, conectores.

### KiCad/
Diseño en KiCad de la [antena IFA ranurada integrada en PCB](KiCad/README.md)
(`ANT_IFA_915MHz_LSM110A`), reconstruida del plano «1.5 Antenna Dimension»: footprint,
símbolo, placa de prueba para ajuste con NanoVNA y generadores paramétricos. Todo se
regenera y verifica con `KiCad/tools/build.sh` (122 comprobaciones, DRC = 0).
**Ojo:** es un redibujo, no el Gerber oficial de SJI — ver el aviso de certificación del
README.

### validation/
Evidencia de los gates de validación de hardware. `gate2-tx-pulse/` contiene la
[guía de fuente variable y emulación de CR2450](validation/gate2-tx-pulse/setup-fuente-variable.md)
para medir el pulso TX (issue #6); el reporte GO/NO-GO va en `REPORT.md`.

## Consideraciones de Diseño

- El LSM110A integra el STM32WL (Cortex-M4 + radio Sub-GHz), eliminando la necesidad de un módulo RF externo
- Programación vía ST-LINK externo o UART para reducir costo del BOM
- Diseño EMC/EMI según guía de ST para RF layout optimizado
- Antena: **IFA ranurada integrada en PCB, 902–928 MHz** (Sigfox RC2/RC4 — no 868 MHz, que es RC1).
  Réplica del patrón `EVB_LSM ANT` de SJI, ya reconstruida y verificada en `KiCad/`. Sigue abierta
  la decisión IFA integrada vs. antena externa u.FL, que depende del tamaño de caja: la antena
  fuerza **50 mm** en un eje (ver `antena-cotas.md` §7)
