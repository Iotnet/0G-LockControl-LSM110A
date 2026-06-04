# LSM110A — Footprint EasyEDA

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
| Pad central (GND) | Si (exposed pad) |
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
- El pad central (exposed pad) es GND y debe conectarse al plano de tierra con vias termicas.
- Usar este footprint como base para el layout del PCB del proyecto 0G LockControl.

## Fuente

Exportado desde EasyEDA Pro, proyecto `LSM_Module`.
