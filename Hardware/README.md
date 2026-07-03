# Hardware

Diseño electrónico del dispositivo 0G LockControl basado en el SoC LSM110A.

## Subcarpetas

### Schematic/
Esquemáticos del circuito. Incluye el sistema mínimo del LSM110A: alimentación (LDO), oscilador, protección ESD, conector de programación (SWD/UART), reed switch, y matching network de la antena.

### PCB/
Diseño de la tarjeta de circuito impreso. PCB de una sola capa para reducir costos. Considera separación entre circuitos digitales y RF, plano de tierra, y rutas críticas aisladas según guía AN5457 de ST.

### BOM/
Lista de materiales con referencias de proveedores (Mouser, DigiKey, LCSC), cantidades, costos unitarios y alternativas.

### Datasheets/
Hojas de datos de los componentes principales: LSM110A, reed switch, regulador LDO, antena, conectores.

### validation/
Evidencia de los gates de validación de hardware. `gate2-tx-pulse/` contiene la
[guía de fuente variable y emulación de CR2450](validation/gate2-tx-pulse/setup-fuente-variable.md)
para medir el pulso TX (issue #6); el reporte GO/NO-GO va en `REPORT.md`.

## Consideraciones de Diseño

- El LSM110A integra el STM32WL (Cortex-M4 + radio Sub-GHz), eliminando la necesidad de un módulo RF externo
- Programación vía ST-LINK externo o UART para reducir costo del BOM
- Diseño EMC/EMI según guía de ST para RF layout optimizado
- Antena: PCB trace antenna o chip antenna (868 MHz) por evaluar
