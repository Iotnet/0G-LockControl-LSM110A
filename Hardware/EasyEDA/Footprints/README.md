# LSM110A — Footprint EasyEDA

> **Superado por la libreria KiCad (decision D-07).** La herramienta de CAD del proyecto es
> **KiCad**, y este JSON no sirve en KiCad. Usar
> [`Hardware/v0-replica-sji/kicad-lib/`](../../v0-replica-sji/kicad-lib/): simbolo de 34
> pines + footprint LGA-34 nativos, generados desde las figuras del DS y cotejados contra
> este archivo (coinciden los 34 pads salvo el pin 2, donde **la version KiCad es la
> correcta** — este JSON lo tiene +0.016 mm fuera de reticula, hallazgo N-07).
>
> Este archivo se conserva como referencia historica y como contraste de verificacion.

Footprint custom del modulo **LSM110A** (SJI / Seongji), exportado desde EasyEDA el 2026-06-04.

## Datos del footprint

| Parametro | Valor |
|---|---|
| Package | LGA-34 (custom) |
| Dimensiones | 14.0 x 15.0 mm |
| Pads totales | 34 |
| Distribucion de pads | 12 (izq) + 10 (abajo) + 12 (der) + 0 (arriba) |
| Tamano de pad | 1.20 x 0.60 mm |
| Pitch | 1.00 mm |
| Pad central | No — el modulo no tiene exposed pad |
| Origen | Centro del componente |

## Archivos

| Archivo | Ubicacion | Descripcion |
|---|---|---|
| `PCB_PCB_LSM_Module_2026-06-04.json` | `Footprints/` | Footprint EasyEDA (JSON) |
| `PCB_PCB_LSM_Module_2026-06-04.png` | `Screenshots/` | Captura del footprint |
| `OBJ_PCB_LSM_Module.obj` + `.mtl` | `3DModels/` | Modelo 3D (OBJ) |
| `OBJ_PCB_LSM_Module_2026-06-04.zip` | `3DModels/` | Modelo 3D comprimido (original) |

## Notas

- Verificado contra el datasheet del LSM110A y el diseno de referencia de SJI.
- **No hay pad central.** El footprint tiene exactamente 34 pads, todos perimetrales
  (12 izq + 10 abajo + 12 der). Verificado por conteo directo sobre
  `PCB_PCB_LSM_Module_2026-06-04.json`: 34 entradas `PAD`, numeradas 1..34, sin
  ninguna en el centro del cuerpo.
- **No meter cobre bajo el modulo.** El DS 5.4 pide PSR coating en esa zona, asi que
  un plano de tierra con vias termicas bajo el cuerpo es justo lo contrario de lo
  que pide el fabricante. (Hallazgo N-01, fase F1.)
- Usar este footprint como base para el layout del PCB del proyecto 0G LockControl.

## Fuente

Exportado desde EasyEDA Pro, proyecto `LSM_Module`.
