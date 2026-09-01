# Decisiones técnicas — 0G LockControl

Registro de decisiones arquitectónicas del proyecto. Cada entrada documenta el contexto, las opciones evaluadas y la decisión final.

## DT-001: Sensor magnético — Reed switch + Hall (ambos)
- **Fecha:** 2026-05-07
- **Contexto:** Necesitamos detectar apertura de puerta. Reed es más barato (0µA), Hall (DRV5032) es más robusto y SMD.
- **Decisión:** Poner ambos footprints en la PCB. Mismo pin PA1 (EXTI1). Probar en prototipo.
- **Consecuencia:** Layout de PCB necesita espacio para ambos footprints.

## DT-002: Batería — CR2450 + cap soporte 470µF
- **Fecha:** 2026-05-07
- **Contexto:** Necesitamos batería desechable, tamaño <5cm. CR2450 tiene resistencia interna alta para pulsos de TX Sigfox.
- **Decisión:** CR2450 + capacitor 470µF de soporte. Si falla, pivot a CR2477 (mismo diámetro).
- **Consecuencia:** Validar con osciloscopio que VDD no cae debajo de 1.8V durante TX.

## DT-003: LDO — Footprint con bypass 0Ω
- **Fecha:** 2026-05-07
- **Contexto:** Alimentar directo desde CR2450 (3V→2V) maximiza vida útil. Con LDO a 2.5V se pierde headroom.
- **Decisión:** Footprint de TPS7A02 con bypass de 0Ω. Probar ambas configs sin re-hacer PCB.
- **Consecuencia:** BOM tiene componente opcional.

## DT-004: Firmware — API (sin MCU externo)
- **Fecha:** 2026-05-07
- **Contexto:** AT commands requiere MCU externo. API permite programar directo en el STM32WL del LSM110A.
- **Decisión:** Firmware API. Código en C corre dentro del módulo. Stack Sigfox de SJI no se modifica.
- **Consecuencia:** Curva de aprendizaje en C bare-metal. Mayor control de bajo consumo.

## DT-005: Potencia TX — +14dBm default, configurable
- **Fecha:** 2026-05-07
- **Contexto:** +14dBm consume ~50mA (seguro con CR2450). +22dBm consume ~123mA (riesgoso).
- **Decisión:** Default +14dBm, configurable en firmware. Subir solo si cobertura insuficiente.
- **Consecuencia:** Alcance de 2-5km en zona urbana. Suficiente para interiores con cobertura Sigfox.

## DT-006: Heartbeat — Cada 24 horas
- **Fecha:** 2026-05-07
- **Decisión:** 1 mensaje keepalive/día con: nivel batería, temperatura, conteo eventos.
- **Consecuencia:** Máxima vida de batería. Detección de falla en máximo 24h.

## DT-007: Cooldown — 60 segundos entre mensajes
- **Fecha:** 2026-05-07
- **Decisión:** Mínimo 60s entre transmisiones + contador diario max 130 msgs (guarda 10 para heartbeats).
- **Consecuencia:** Protege límite Sigfox de 140 msgs/día.

## DT-008: Certificación NOM — Post-MVP
- **Fecha:** 2026-05-07
- **Decisión:** NOM-208/IFT-008 aplica para 902-928 MHz. LSM110A tiene FCC + MRA México-USA. Gestionar después del MVP.
- **Consecuencia:** No bloquea prototipado. Necesario para comercialización.

## DT-009: Watchdog IWDG como red de seguridad
- **Fecha:** 2026-05-07
- **Contexto:** Con firmware API sin MCU externo, un bug en el código de aplicación puede colgar todo el sistema.
- **Decisión:** Habilitar IWDG (Independent Watchdog) con timeout de 4 segundos. Refresh en el loop principal.
- **Consecuencia:** El dispositivo se recupera automáticamente de cuelgues de firmware en máximo 4 segundos.

## DT-010: Antena — diseño de referencia SJI sin modificaciones
- **Fecha:** 2026-05-07
- **Contexto:** La certificación FCC del LSM110A requiere usar exclusivamente la antena tipo traza PCB diseñada por SJI. Cualquier cambio de antena invalida la FCC.
- **Decisión:** Copiar exactamente el diseño de antena del EVB de SJI. Agregar conector U.FL como opción de pruebas con jumper 0Ω.
- **Consecuencia:** El layout de la antena PCB no puede modificarse sin re-certificar. El conector U.FL es solo para pruebas de desarrollo.
- **Referencia:** FCC ID: 2AS8LLSM110A — fccid.io/2AS8LLSM110A. Grant verificado del
  **2022-06-14**, titular **SJI CO. LTD**, y es para el LSM110A. **Pero el datasheet R08 §7
  declara otro ID** (`2BEK7LSM110A`, SJIT Co. Ltd) — discrepancia abierta, ver **DT-011b**.
  No cambiar este ID hasta cerrarla.
- **Actualización (datos FCC verificados, doc 5937666):** La referencia SJI usa CPWG 50Ω (1.0 mm / 0.15 mm), no microstrip. Matching: L101=0Ω, C101=2.2pF, C102=DNI. Placa de referencia 50×80 mm, εr 4.3. Separación RF mínima 20 cm a personas (requisito de certificación). El Gerber de la antena es confidencial — en gestión con GREATECH/SJI bajo NDA.

## DT-011: Certificaciones — plan de cumplimiento
- **Fecha:** 2026-05-07
- **Contexto:** Para vender el dispositivo en México se necesitan: Sigfox Ready, NOM-208/IFT-008, y suscripción Sigfox.
- **Decisión:** Todas las certificaciones se tramitan post-MVP. El diseño cumple desde ahora las condiciones para no invalidar certificaciones heredadas.
- **Condiciones para preservar certificaciones:**
  1. No modificar la stack de Sigfox del SDK de SJI
  2. Usar antena PCB del diseño de referencia SJI
  3. No cambiar frecuencia de operación (RC2, 902-928 MHz)
  4. No exceder potencia máxima certificada (+22dBm)
  5. Etiquetar producto con "Contains FCC ID: <el que confirme el proveedor>" — ver la
     actualización de abajo; el ID exacto está sin cerrar.

- **Actualización (2026-08-20, hallazgo H-03) — qué FCC ID va en la etiqueta, sin resolver.**
  Afecta solo a la serigrafía/etiqueta final (F6). Hay **dos FCC ID** en circulación para el
  LSM110A, y son dos *grantee codes* distintos, no un typo:

  | Código | Titular | Estado verificado |
  |---|---|---|
  | `2AS8LLSM110A` | **SJI CO. LTD** | Grant **verificado**, del **2022-06-14**, y **es para el LSM110A** |
  | `2BEK7LSM110A` | **SJIT Co. Ltd** | El grantee code `2BEK7` tiene grants reales, pero **ninguno del LSM110A localizable** |

- **Lo que dice cada fuente:** el `DS_LSM110A_R08` §7 (pág. 20) declara
  `2BEK7LSM110A` + IC `32019-LSM110A` + ANATEL `05243-24-12325`. Este repo usa
  `2AS8LLSM110A`, que es el único de los dos con grant localizable para este módulo.
- **Por tanto:** la hipótesis de un cambio de titular SJI → SJIT es **plausible pero no
  confirmada**. Que el datasheet sea más nuevo no basta: no hay grant de LSM110A bajo
  `2BEK7` que se pueda abrir.
- **Decisión provisional:** **no se cambia ningún ID en el repo todavía.** Y en v0 se usa
  **etiqueta adhesiva, no serigrafía** — si se serigrafía el ID equivocado hay que
  re-fabricar; una etiqueta se cambia.
- **Pendientes para cerrarlo:**
  1. Búsqueda en el EAS de la FCC: <https://apps.fcc.gov/oetcf/eas/reports/GenericSearch.cfm>
     (buscar por grantee code `2BEK7` y por producto `LSM110A`).
  2. Preguntar a GREATECH / SJI qué FCC ID e IC ID amparan **el lote comprado**, y pedir
     el PDF del grant y el marcado del módulo.
- **Lo que NO bloquea:** la compra de U1 (el part number es `WSLSM110A00` y no tiene
  variante por grant), el esquemático, la BOM, el layout de cobre ni el firmware.

---

> **DT-012 a DT-016 vienen de la fase F1** de la réplica v0 del diseño SJI, donde se
> tomaron con las etiquetas `D-04`…`D-08`. Se renumeran aquí a la serie `DT-` porque
> **este archivo es el registro único de decisiones**. La equivalencia queda en
> `Hardware/v0-replica-sji/ESTADO.md`. Las `D-01`…`D-03` no se trasladan: son de
> proceso de trabajo (cómo se hace push, cómo se abren los PR, dónde se clona), no de
> diseño, y viven solo en `ESTADO.md`.

## DT-012: Pinout — el documento viejo se reemplaza, no se parchea
- **Fecha:** 2026-08-20 · **Fase:** F1 · *(era `D-04`)*
- **Contexto:** `docs/pinout-lsm110a.md` tenía 4 pines mal (H-01) y **nunca se derivó de la
  Tabla 5-1-1** del datasheet. Parchear 4 celdas deja el resto sin procedencia conocida.
- **Decisión:** la fuente de verdad del pinout es
  `Hardware/v0-replica-sji/00-fuente-de-verdad/pinout-34-pines.md` §1, derivada de la Tabla
  5-1-1 del DS R08 (págs. 14–15) y verificada contra 3 fuentes. `docs/pinout-lsm110a.md` se
  reemplaza entero y pasa a ser un subconjunto derivado que apunta a ella.
- **Consecuencia:** si los dos documentos discrepan, manda `pinout-34-pines.md`. Los errores
  eran PA9 26→3, PA10 27→4, PA0 14→16, PA2 16→14; los dos últimos eran un intercambio
  PA0↔PA2 que ponía el LED de debug sobre `UART2_TX`, el puerto de rescate por IAP.

## DT-013: Presupuestos con valores MAX, no typ
- **Fecha:** 2026-08-20 · **Fase:** F1 · *(era `D-05`)*
- **Contexto:** los presupuestos de consumo se habían hecho con valores *typical*. H-12.
- **Decisión:** donde el datasheet publique `max`, los presupuestos usan `max`. Es política
  general, no una corrección puntual.
- **Consecuencia:** el consumo en sleep se presupuesta a **5 µA (max)**, no a 1.8 µA (typ).
  Afecta a la vida de batería calculada y al criterio de GATE 2 en F2.

## DT-014: El MCU del módulo es STM32WLE5CC
- **Fecha:** 2026-08-20 · **Fase:** F1 · *(era `D-06`)*
- **Contexto:** el repo decía `STM32WL55*` en ~20 sitios. Ese es el MCU de la placa
  **Nucleo-WL55JC** del prototipo previo, y se arrastró al repo del módulo.
- **Decisión:** el MCU dentro del LSM110A es **`STM32WLE5CC`** (DS R08 §1.1, pág. 4 — única
  mención de un part number de MCU en los 11 PDF del fabricante).
- **Consecuencia:** el `WL55JC` es **dual-core** (M4 + M0+) y el `WLE5CC` es **single-core**
  M4; no son intercambiables al hablar de arquitectura. Flash/RAM también difieren, y los
  256 kB de la letra `C` son los que cuadran con el mapa de memoria. En bring-up,
  STM32CubeProgrammer reportará la familia **STM32WLE5**. Las referencias a
  `NUCLEO-WL55JC` / `STM32WL55JCI` del trabajo previo son correctas y **no** se tocan.

## DT-015: La herramienta de CAD es KiCad, no EasyEDA
- **Fecha:** 2026-08-20 · **Fase:** F1 · *(era `D-07`)*
- **Contexto:** la guía de fases está escrita mezclada (§2 y §5 dicen EasyEDA, §4 dice
  KiCad) y el repo tiene un footprint en JSON de EasyEDA.
- **Decisión:** **manda KiCad.**
- **Consecuencias:**
  - El JSON de `Hardware/EasyEDA/Footprints/` **no sirve** en KiCad. Sustituido por librería
    nativa en `Hardware/v0-replica-sji/kicad-lib/` (símbolo de 34 pines + footprint LGA-34),
    generada desde las figuras del DS y cotejada contra el JSON: coinciden los 34 pads salvo
    el pin 2, donde **la versión KiCad es la correcta** (N-07).
  - Los entregables de F3/F5/F6 cambian de formato: `.kicad_sch`, `.kicad_mod` con polígonos
    de cobre para la antena, y reglas de DRC de KiCad.
  - `kicad-cli` permite correr **ERC y DRC por línea de comandos**, y los archivos son texto
    revisable. Mejora el bucle de validación de F3 y F6.
  - **No** cambia: JLCPCB como casa de fabricación, la BOM contra LCSC, el stackup de 1.6 mm
    ni el recálculo de impedancia pendiente (deuda B-03).

## DT-016: Orden real de ejecución de las fases
- **Fecha:** 2026-08-20 · **Fase:** F1 · *(era `D-08`)*
- **Contexto:** la guía recomienda hacer F2 (energía) justo después de F1, por ser la que más
  riesgo elimina por hora invertida. Pero F2 es **trabajo de banco** y falta hardware: CR2450
  reales y una pila descargada a ~2.4 V para el caso peor.
- **Decisión:** el orden es **F3 → F4 → (F2 cuando lleguen las pilas) → F5 → F6 → F7**.
- **Consecuencia:** F2 no bloquea a nadie, pero hay un acoplamiento: **F2 puede forzar
  cambios en F3** (valor y tecnología del condensador de soporte, y el posible pivot a
  CR2477). Por eso F3 debe dejar ese footprint dimensionado con holgura y **no cerrar la BOM
  de alimentación** hasta tener GATE 2.

## DT-017: Acelerómetro — se confirma LIS2DW12; segunda fuente y plan de compra
- **Fecha:** 2026-09-01
- **Contexto:** el LIS2DW12 se eligió al inicio del proyecto sin una comparativa documentada.
  Antes de cerrar la BOM conviene verificar que sigue siendo la parte correcta, y resolver
  dónde se compra: **ni UNIT Electronics ni AG Electrónica venden MEMS sub-µA en LGA-12** —su
  catálogo de movimiento son breakouts (ADXL345 GY-291, ADXL335 GY-61, MPU6050, BMI160/270,
  MPU-9250, BNO085), todos por encima del presupuesto de energía.
- **Filtro aplicado:** ≤1 µA con detección activa (presupuesto de sleep de 5 µA, DT-013) ·
  1.7–3.6 V funcionando a 2.0 V (CR2450 directa, DT-003) · I²C (PA9/PA10 son los únicos pines
  cableados) · INT por umbral hacia PA0/EXTI0 · motion engine que reporte eje dominante
  (bytes 2–4 del payload) · ≤2×2 mm y <1 USD @1k.
- **Decisión:**
  1. **Producción: se mantiene `U2 = LIS2DW12TR`** (LCSC C189624). Es el único candidato que
     cumple los seis requisitos a la vez, y ya está integrado en esquemático, BOM, pinout y
     driver validado (Y1–Y3).
  2. **Segunda fuente pin-compatible: `LIS2DTW12`**, que además trae termómetro absoluto
     ±0.8 °C typ —relevante porque el byte 7 del payload es temperatura y hoy se lee del
     sensor interno del LIS2DW12, que es de compensación de offset, no de medición absoluta.
     Se conserva `LIS2DH12` como respaldo barato por quiebre de stock.
  3. **Alternativa para v1 (no drop-in): `BMA400`** de Bosch — 800 nA, auto-wakeup por
     hardware, orientado a cerraduras y sensores de puerta. **Pinout distinto → exige respin.**
  4. **Descartado `ADXL362`** pese a ser el de menor consumo (270 nA): es **solo SPI**, obliga
     a re-rutear y a tirar el driver. Queda como contingencia si el GATE 2 no cierra.
  5. **Compra:** el chip va por LCSC (integra con el ensamble JLCPCB) o, en canal nacional con
     factura, DigiKey México / Mouser / Arrow es-mx. **Para banco se compra en UNIT el
     ADXL345 GY-291 (~$70 MXN)** para no bloquear Y4 esperando importación: valida la cadena
     I²C+EXTI, **no** el consumo. El consumo se mide con `STEVAL-MKI179V1` o con LIS2DW12TR
     sueltos en breakout propio.
- **Consecuencia:** cero impacto en esquemático, layout, BOM (fila U2) y firmware. Solo se
  añade `LIS2DTW12` a la columna de sustitutos y una compra menor de banco.
- **Pendientes:** confirmar VDD/pinout del BMA400; verificar `WHO_AM_I` y registros de
  temperatura del LIS2DTW12; fijar el consumo exacto del LIS2DW12 a 12.5 Hz en LP mode 1 con
  el margen que exige DT-013 (el fabricante no publica `max`).
- **Análisis completo:** [`seleccion-acelerometro.md`](./seleccion-acelerometro.md)
