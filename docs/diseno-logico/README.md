# Diseño lógico del firmware — FSM, clasificación de eventos y payload

Documento de diseño de **0G LockControl**: entradas físicas, variables de estado,
eventos, máquina de estados, lógica de clasificación (tabla de verdad + Karnaugh)
y la **extensión propuesta al payload** de 12 bytes.

- **Autor:** José Francisco Díaz — I+D / Ingeniería, 0G IoT Solutions
- **Versión:** 0.1 (borrador de diseño) · 23 de julio de 2026
- **Relación con WP-01:** complementa y **extiende** el contrato del payload.
  El contrato vigente y la propuesta conviven documentados en
  [`../PAYLOAD-DEFINICIONES.md`](../PAYLOAD-DEFINICIONES.md).

> Es un documento de **diseño**, no de implementación. Nada de lo que propone
> está todavía en `payload_codec.{h,c}` ni en `payload_parser.py`.
> El contrato que sale de aquí es la **v2** y encabeza
> [`PAYLOAD-DEFINICIONES.md`](../PAYLOAD-DEFINICIONES.md) (§1–§3); lo que el
> código hace hoy (v1) está en §4, y los pendientes en §8.

---

## Contenido de esta carpeta

| Archivo | Qué es |
|---|---|
| `lockcontrol_diseno.pdf` | **Documento compilado** — empieza aquí para leerlo |
| `lockcontrol_diseno.tex` | Fuente LaTeX (755 líneas, 13 secciones) |
| `generar_diagramas.py` | Genera las 5 figuras con matplotlib (paleta corporativa 0G) |
| `figs/casos_uso.png` | Diagrama de casos de uso (UML) |
| `figs/estados_fsm.png` | Máquina de estados del firmware |
| `figs/secuencia.png` | Diagrama de secuencia (escenario vandalismo) |
| `figs/karnaugh.png` | Mapas de Karnaugh de la clasificación |
| `figs/cronograma.png` | Cronograma temporal (ventana 10 s + cooldown 60 s) |

Las figuras van en `figs/` porque es la ruta que usan tanto el `.tex`
(`\includegraphics{figs/...}`) como el script (`FIGDIR = ./figs`). En el zip
original venían planas junto al `.tex`, así que **tal cual no compilaba**.

## Regenerar las figuras

```sh
pip install matplotlib numpy
python3 generar_diagramas.py     # reescribe los 5 PNG en ./figs/
```

## Recompilar el PDF

```sh
pdflatex lockcontrol_diseno.tex   # 2 pasadas: la 1.ª arma el índice y las refs
pdflatex lockcontrol_diseno.tex
```

Paquetes LaTeX que usa: `inputenc`, `fontenc`, `geometry`, `amsmath`, `amssymb`,
`graphicx`, `float`, `caption`, `xcolor` (`dvipsnames,table`), `booktabs`,
`tabularx`, `array`, `enumitem`, `fancyhdr`, `listings`, `titlesec`, `hyperref`,
`textcomp`. Con una TeX Live / MacTeX completa están todos.

> La fecha del documento está fijada a mano (`\renewcommand{\today}{23 de julio
> de 2026}`), así que recompilar no la mueve. Actualizarla al cambiar de versión.

---

## Resumen del diseño (para no abrir el PDF)

### Tres entradas

| Entrada | Componente | Pin / IRQ | Rol |
|---|---|---|---|
| Botón de armado | Push-button | **sin asignar** (falta GPIO con EXTI libre) | Arma / desarma. Llave maestra. |
| Apertura | Reed switch + DRV5032 | PA1 / EXTI1 | Flancos de apertura/cierre; fuente de `N` |
| Movimiento | LIS2DW12 (I²C) | PA0 / EXTI0 (INT1) | Desplazamiento `θ` vs `accel_ref` |

### Regla de oro

**Sólo se transmite si el sistema está armado.** Guarda global: con
`estado_fsm == DESARMADO` se ignora todo flanco del Hall y del acelerómetro.
Evita inundar Sigfox con el tránsito normal de la casa.

### FSM

```
DESARMADO → (botón, captura accel_ref) → ARMADO → (flanco apertura, t0=RTC, N=1)
          → OBSERVANDO (T_OBS = 10 s) → CLASIFICAR → TRANSMITIR → ARMADO
```

`ARMADO` es reposo en **Stop2 (~3 µA)**; sólo despierta por Hall, acelerómetro
o el timer de heartbeat.

### Clasificación — tres booleanas, tres salidas excluyentes

```
C = (N > N_umbral),  N_umbral = 5          "muchos ciclos"
M = (θ_max ≥ θ_umbral)                     "movimiento real"
P = puerta abierta al cierre de la ventana "estado final"
```

Ecuaciones minimizadas con Karnaugh (los 8 casos quedan cubiertos, sin huecos
ni solapes):

```
VANDALISMO: V = C · !M
APERTURA:   A = P · (!C + M)
CIERRE:     Z = !P · (!C + M)
```

El criterio de vandalismo: **muchos flancos del Hall sin que el acelerómetro
confirme que la puerta se abrió de verdad** — alguien forcejeó la puerta cerrada,
el reed castañeteó pero la hoja no giró.

### Dos temporizadores que no hay que confundir

| Temporizador | Valor | Propósito |
|---|---|---|
| Ventana de observación `T_OBS` | **10 s** | Agrega la ráfaga en un evento, mide `N` y `θ_max` |
| Cooldown de TX | **60 s** (ya decidido) | Anti-flood, protege el presupuesto Sigfox (130 msg/día) |

Se descarta una ventana de 60 s: retrasaría la alerta hasta un minuto y acoplaría
"observación" con "rate limit", que son cosas distintas. El presupuesto ya lo
cuida el cooldown.

### Medir el tiempo abierto: sí, pero con RTC

| Puerta abierta | RTC + Stop2 (~3 µA) | Timer GP + Run (~2 mA) | Factor |
|---|---|---|---|
| 1 minuto | ~0.00005 mAh | ~0.033 mAh (≈1 TX) | ~670× |
| 1 hora | ~0.003 mAh | ~2 mAh (≈72 TX) | ~670× |
| 8 horas | ~0.024 mAh | ~16 mAh (≈576 TX) | ~670× |

Se guarda `t0 = RTC` en el flanco de apertura, **el MCU vuelve a Stop2**, y al
flanco de cierre se calcula `t_ab = RTC - t0`. Medir cuesta dos lecturas de
registro. Se descarta el timer de propósito general con el MCU despierto.

### Decisiones y descartes

| Tema | Decisión |
|---|---|
| Guarda de armado | ✅ Sí |
| Ventana `T_OBS` | ✅ 10 s (parametrizable, `t_obs_ms`) |
| Ventana de 1 min | ❌ Descartada |
| Cooldown | ✅ 60 s (ya decidido en el proyecto) |
| Umbral de ciclos `N` | ✅ 5 |
| Umbral `θ` | ✅ Reusar los 200 mg del acelerómetro; calibrar en campo |
| Medir `t_ab` | ✅ Sí, vía RTC |
| Timer GP + MCU en Run | ❌ Descartado (consumo en mA) |
| Hora en el payload | ❌ Descartada — la aporta el backend Sigfox |
| Contador monotónico en la app | ❌ Descartado — Sigfox ya da secuencia de red; se libera `conteo` para `N` |

---

## Pendientes que deja el documento

1. Asignar el **GPIO del botón** (EXTI libre) en CubeMX y su antirrebote.
2. **Calibrar** `θ_umbral` y `debounce_ms` con la puerta real.
3. Actualizar el **payload** (bytes 10–11 + semántica de 2–3 y 8–9) en el codec
   y el parser, y recalcular los vectores V1–V3.
4. Documentar la lógica en el **Notion Hub** del proyecto.

El detalle del punto 3 está en el **checklist de migración (§8)** de
[`../PAYLOAD-DEFINICIONES.md`](../PAYLOAD-DEFINICIONES.md).

> Al analizar el layout byte por byte salió un desbalance que conviene resolver
> **antes** de implementar: `N` se queda con 16 bits para un umbral de decisión de
> 5, mientras `t_abierto_s` satura en 255 s (4 min 15 s) y no puede reportar
> "abierta toda la noche" — el caso futuro que este mismo documento propone.
> Análisis completo y corrección posible en §2.4 de `PAYLOAD-DEFINICIONES.md`.
> **El layout de §1 no se cambió**: la decisión es tuya / de Franco.

## Casos futuros contemplados

Caben en el mismo byte `evento` sin cambiar el contrato:

- **Batería baja** — al cruzar un umbral; puede piggy-back en el heartbeat.
- **Sabotaje / tamper** — golpe detectado por el acelerómetro con la puerta
  *cerrada*. Distinto del vandalismo de puerta.
- **Puerta abierta prolongada** — recordatorio si `t_ab` supera un límite; con
  cuidado por el presupuesto Sigfox.

---

*Nota: el antirrebote del Hall (`debounce_ms`) no es opcional. Un reed switch
rebota, y sin filtrar, **una** apertura real podría contarse como varios flancos
y disparar un "vandalismo" falso. Es la salvaguarda que hace confiable a `N`.*
