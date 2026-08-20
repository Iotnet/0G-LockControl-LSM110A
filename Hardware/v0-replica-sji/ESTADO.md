# ESTADO — v0 réplica SJI

Última actualización: **2026-08-20** por **F1 (Fuente de verdad)** · sesión Cowork
Rama de trabajo: `v0/f1-fuente-de-verdad` → integra en `feature/v0-replica-sji`

| Fase | Estado | Cerrada | Commit | Notas |
|------|--------|---------|--------|-------|
| F0 Setup            | ✅ | 2026-08-20 | *(pendiente de push)* | Ejecutado dentro de la sesión de F1: no existía. Revisiones confirmadas. |
| F1 Fuente de verdad | ✅ | 2026-08-20 | *(pendiente de push)* | 6 `.md` completos. Resuelve H-01, H-04, H-13. Cuantifica H-05. |
| F2 Energía          | ⬜ | — | — | Bloqueada por hardware: faltan CR2450 reales y una descargada a ~2.4 V. No bloquea a F3. |
| F3 Esquemático      | ⬜ | — | — | **Siguiente que hará Franco.** Desbloqueada por F1 + librería KiCad. |
| F4 BOM              | ⬜ | — | — | |
| F5 Antena           | ⬜ | — | — | Desbloqueada por F1, con deuda declarada (ver B-01). |
| F6 Layout           | ⬜ | — | — | |
| F7 Fabricación y bring-up | ⬜ | — | — | |

---

## Verificación de fuentes (F0, paso 2)

Ambos repos se clonan sin problema **desde el sandbox de Cowork** (R4 confirmado; `git clone` funciona, no hace falta `curl`).

| PDF | Revisión esperada | Verificado | SHA-256 |
|---|---|---|---|
| `DS_LSM110A_R08_241008.pdf` | R08, 24 págs | ✅ **24 págs** | `76abc36b9b1944b1144ae15f99bb6a38a378538c4d8f6b2f0175b722301f04b6` |
| `[SJIT]_LSM110A_UserManual_Rev1.4_240626.pdf` | Rev 1.4, 33 págs | ✅ **33 págs** | `02cb31b454a7e86d58b721f02134c4715715f181c5efb49bb288d4e4d6de6334` |
| `user-manual-antenna-trace-design.pdf` (FCC, repo propio) | — | ✅ 9 págs | `bf344d65f2a8c5b021d49e1ea858da03783482ddf5082b4c086eba9458e2e116` |

**No hay revisión más nueva publicada.** No hay que parar. Copias en `00-fuente-de-verdad/pdfs/`.

---

## Decisiones tomadas

- **D-01 (F0):** push **manual**, sin token. Cada fase deja un `COMMIT-Fn.md` con los comandos git exactos, que Franco ejecuta con Claude Code. *(decidido por Franco, 2026-08-20)*
- **D-02 (F0):** **PR por fase**, de `v0/f1-fuente-de-verdad` hacia `feature/v0-replica-sji`. *(decidido por Franco)*
- **D-03 (F0):** los repos **no** se clonan en la máquina de Franco. Se clonan en el sandbox en cada sesión (tarda segundos y funciona). La carpeta local `PCB\` es **destino de entregables**, no espejo del repo. Los entregables se escriben en ruta espejo del repo para poder copiarlos y commitear sin renombrar. *(decidido por Franco)*
- **D-04 (F1):** el pinout de `docs/pinout-lsm110a.md` **se reemplaza**, no se parchea. Nunca se derivó de la Tabla 5-1-1 y tiene 4 errores. Sustituto: `00-fuente-de-verdad/pinout-34-pines.md` §1.
- **D-05 (F1):** donde el DS publique `max`, los presupuestos usan `max`. Cierra H-12 como política, no como corrección puntual.
- **D-06 (F1):** el MCU del módulo es **STM32WLE5CC** (DS §1.1, pág. 4). Todas las referencias del repo a `STM32WL55*` se corrigen.
- **D-07 (F1):** la herramienta de CAD es **KiCad**, no EasyEDA. La guía está escrita mezclada (§2 y §5 dicen EasyEDA, §4 dice KiCad); **manda KiCad**. *(confirmado por Franco, 2026-08-20)*
  Consecuencias, y son buenas:
  - El JSON de `Hardware/EasyEDA/Footprints/` **no sirve** en KiCad. Sustituido por librería nativa en `kicad-lib/` (símbolo de 34 pines + footprint LGA-34), generada desde las figuras del DS y **cotejada** contra el JSON de EasyEDA: coinciden los 34 pads salvo el pin 2, donde la versión KiCad es la correcta.
  - Los entregables de F3/F5/F6 cambian de formato: `.kicad_sch`, `.kicad_mod` con polígonos de cobre para la antena, y reglas de DRC de KiCad. Ya no hace falta «JSON importable a ciegas».
  - `kicad-cli` permite correr **ERC y DRC por línea de comandos**. El bucle de la guía §4 («Franco pega el reporte») se puede sustituir por revisión directa del `.kicad_sch` / `.kicad_pcb`, que es texto. Mejora el bucle de validación de F3 y F6.
  - Lo que **no** cambia: JLCPCB como casa de fabricación, la BOM contra LCSC, el stackup de 1.6 mm y el recálculo de impedancia de B-03.
- **D-08 (F1):** el orden real de ejecución es **F3 → F4 → (F2 cuando lleguen las pilas) → F5 → F6 → F7**, no el de la guía. Razón: F2 es trabajo de banco y falta hardware; F3 es CAD y está desbloqueada. F2 sigue sin bloquear a nadie, pero F3 debe dejar el condensador de soporte dimensionado con holgura y **no cerrar la BOM de alimentación** hasta tener GATE 2.

---

## Preguntas abiertas para Franco

Ninguna de F1 — la fase es extracción, no decisión. Estas quedan **pendientes para sus fases**:

**Para F2 (energía):**
- ¿Qué condensador de soporte se prueba primero: 470 µF tántalo (como la BOM) o polímero low-ESR? El ESR del tántalo estándar (~400 mΩ) importa en un pulso de 123 mA.
- ¿Se prueba también la configuración con LDO (`U4a` TPS7A02), o solo el bypass directo de 0 Ω?
- ¿Hay pilas de repuesto y una descargada a ~2.4 V para el caso peor? Si no, ¿se emula con fuente + resistencia serie que imite el ESR?

**Para F3 (esquemático):**
- **Supervisor de reset de 1.8 V: ¿se pobla?** F1 recomienda **sí**, con argumento en `limites-electricos.md` §5.1 (riesgo de corrupción de flash al final de la vida de la pila). Tiene coste: I_Q continua del supervisor, que se suma al presupuesto de F2.
- **Pull-ups I2C:** el repo dice 4.7 k, la BOM dice 10 k. Elegir uno y alinear ambos documentos.
- **Variante del DRV5032:** F1 recomienda **push-pull** (`DRV5032FB`), para eliminar el pull-up de PA1. Ver `limites-electricos.md` §6: un pull-up de 10 k en bajo son 300 µA, 60× el consumo del módulo completo.
- **u.FL:** ¿se pobla en v0? Recomendado sí, con selector de 0 Ω. Ojo: la referencia deja antena PCB y SMA en paralelo permanente y los separa **cortando pista** (`red-rf.md` §1); para v0 hace falta un selector real, no un corte.
- **LED de debug:** ¿se pobla? ¿en qué pin? **No en PA2** (es UART2_TX, el puerto del bootloader IAP). El diseño de referencia usa PA8 (pin 24), PA11 (pin 5) y PA15 (pin 9) para sus tres LEDs — los tres libres para nosotros.
- **Red RF: ¿la del EVB (certificada por el exhibit FCC) o la del DS §6.1 (con protección ESD)?** F1 recomienda poblar los footprints de las dos y montar la del EVB en v0, dejando C3/L1 como opción. Ver `red-rf.md` §2.

**Para F5 (antena):**
- Confirmar que se acepta el tamaño: la pestaña de antena mide **50.00 × 20.03 mm** y la placa de referencia es 50 × 80 mm. Rompe el objetivo de <5 cm, que es lo que v0 acordó sacrificar — pero conviene reconfirmarlo **antes** de rutear.
- ¿v0 necesita la pestaña desprendible? Permite medir en conducido (útil para separar «antena mal» de «radio mal»), pero mete una discontinuidad mecánica a 5.60 mm del feed.

**Para F7 (bring-up):**
- ¿El ID/PAC de los módulos del lote ya está registrado en el backend RC2 (UnaBiz / WND México)? Las credenciales vienen de fábrica, pero estar grabadas ≠ estar registradas, y el criterio de cierre de v0 es un uplink **visible en el backend**.

---

## Bloqueos

Ninguno.

---

## Verificación de F1 (auditoría automatizada, 2026-08-20)

Pasada de verificación sobre los 6 entregables, ejecutada por código contra los PDFs.

| Qué se comprobó | Método | Resultado |
|---|---|---|
| Las **34 filas del pinout** | extracción independiente del PDF con regex + comparación fila a fila | **0 discrepancias** |
| Los 7 pines GND | derivado de la extracción | ✅ `1,10,12,20,23,32,34` |
| Ausencia de pad central | búsqueda de `central`/`exposed` en DS págs. 14–16 | ✅ 0 coincidencias |
| Z₀ CPWG = 51.22 Ω | recálculo Wadell/Ghione | ✅ exacto |
| Pull-up 10 k = 300 µA → 86.1 días | recálculo | ✅ exacto |
| Tiempo de aire 0.160 / 0.480 s | recálculo | ✅ exacto |
| Vida sleep 14.15 / 39.29 años | recálculo | ✅ exacto |
| Conversión RL ↔ VSWR (N-02) | recálculo sobre los 5 marcadores | ✅ confirma −31.69 dB y **descarta** −37.696 |
| **25 citas documento+página** | apertura programática de cada página citada y búsqueda del dato | **24 correctas, 1 mal dirigida** |

**Defecto encontrado y corregido:** la secuencia IAP de `mapa-memoria.md` §4 citaba
*«FW Download Guide §1.1, págs. 3–4»*. Las páginas reales son **4–6**
(115200 y reset+`1` en la pág. 4, 9600 en la 5, `AT$RFS` en la **6**).
El **dato era correcto, la página no**. Corregido, y ahora cada paso lleva su página individual.

**Verdicto: cumple la regla R2** (todo dato con documento y página verificable), tras esa corrección.

**Lo que esta auditoría NO cubre**, y hay que saberlo: las afirmaciones que dependen de
**leer una imagen** no se pueden verificar por código. Quedan respaldadas por una sola
lectura visual y son las candidatas a re-revisar si algo no cuadra más adelante:

- El **orden de la red RF** (C102 → L101 → C101) — leído de la Fig. de FCC pág. 6 y del
  esquemático del UM pág. 5. Dos fuentes independientes coinciden, así que la confianza es alta.
- Los **valores de componente del EVB** (`R8 = 100 k`, `R1 = 390 Ω`, `C2 = DNI/1608`,
  `C3 = 100 nF`, `U2 = CP2104`) — leídos del esquemático a 500 dpi.
- La **red ESD del DS §6.1** (`C3 = 100 pF` serie, `L1 = 47 nH` shunt) — leída de la Fig. 6-1-1.
- **Todas las cotas de la antena** — ver B-01, que ya declara esta deuda de forma explícita.
- Los **valores numéricos de los marcadores de RL/VSWR** — el bitmap embebido está agotado
  a cualquier resolución; N-02 documenta cómo se resolvió la ambigüedad por coherencia
  cruzada entre las dos gráficas en lugar de por lectura directa.

---

## Deuda declarada (no es bloqueo, pero hay que saberlo)

- **B-01 (para F5):** el dibujo acotado de la antena (UM §1.5, pág. 8) **no cierra la geometría de forma única**. No hay datum, dos cotas (`2.00` y `3.03`) son de lectura dudosa incluso a 600 dpi, no está rotulado cuál de los tres verticales es el feed y cuál el cortocircuito, y las cotas verticales legibles no suman los 20.03 mm de forma obvia. F5 **debe reconciliar aritméticamente antes de generar cobre**: si las verticales no cierran, la interpretación está mal. Detalle en `antena-cotas.md` §3.
- **B-02 (para F5/F6):** las ranuras de la pestaña desprendible (5 en el dibujo) **no están acotadas en ninguna fuente**. Hay que dimensionarlas contra las reglas de *tab routing* de JLCPCB, no copiarlas. Y su posición está acoplada a la del feed, porque los tres verticales las cruzan.
- **B-03 (para F6):** los 51.2 Ω de la traza CPWG están calculados con `h = 1.6 mm` y `εr = 4.3`. **Hay que rehacer el número en la calculadora de impedancia de JLCPCB con el stackup real cotizado.** El cálculo de F1 verifica que la geometría de SJI es coherente; no sustituye al del fabricante.
- **B-04 (para F2):** la guía estima «7–9 s de TX» para 3 frames; el cálculo da **0.48 s de payload en el aire**. Discrepancia de más de un orden de magnitud, probablemente por confundir duración de la secuencia con tiempo de transmisión. **F2 lo resuelve con el osciloscopio, no calculando.** Detalle en `limites-electricos.md` §6.

---

## Hallazgos del Apéndice B — estado tras F1

| ID | Fase | Estado tras F1 |
|---|---|---|
| **H-01** pinout con 4 pines mal | F1 | ✅ **RESUELTO**. Confirmado contra 3 fuentes. PA9→3, PA10→4, PA0→16, PA2→14. Los dos últimos eran un intercambio PA0↔PA2. Sustituto en `pinout-34-pines.md` §1. |
| **H-04** `TX_REPEATS = 2` | firmware | ✅ **CONFIRMADO** literal: *«when 0, sends one Tx. when 1, sends three Tx.»* (Sigfox API manual §2.1, pág. 5). Es un flag; `2` está fuera del dominio válido. Corrección en firmware. |
| **H-13** STM32WL55 vs WLE5 | F1 | ✅ **RESUELTO**. DS §1.1 (pág. 4): **`STM32WLE5CC`**, única mención de MCU en los 11 PDFs del fabricante. WL55JC además es dual-core; la guía §3 está mal en ambos términos. El mapa de memoria (250 kB) confirma los 256 kB del `CC`. |
| **H-05** pull-up de PA1 | F2, F3 | 🔄 **CUANTIFICADO**. 10 k → 300 µA (86 días); 100 k → 30 µA; push-pull → 0. La referencia usa 100 k por ir a USB. Decisión en F3, medición en F2. |
| **H-10** footprints de ESD en RF | F3 | 🔄 **LOCALIZADO**. DS §6.1 Fig. 6-1-1: `C3 = 100 pF` en serie + `L1 = 47 nH` en shunt, dentro del recuadro *«Options for ESD»*. Topología completa en `red-rf.md` §2. Refuerzo: el DS §3.1 declara ESD de solo ±2 kV. |
| **H-08** falta supervisor de reset 1.8 V | F3 | 🔄 **DOCUMENTADO con argumento**. DS §6.1 pide `R1 = 100 k` **y** `U2 = 1.8 V Reset_IC`; el EVB no trae ninguno de los dos. Razón cuantificada en `limites-electricos.md` §5.1. |
| **H-11** cálculos a 915 en vez de 902.2 | F5, F6 | 🔄 **ACOTADO**. RC2 = 902.2 ±0.096 MHz (DS §3.4.1). Error de diseñar a 915: **+1.42 %**. Irrelevante para la traza (banda ancha), relevante para la antena (resonante). |
| **H-12** presupuesto con typ | F2 | ✅ **CERRADO como política** (D-05). Sleep: **5 µA max**, no 1.8 typ. |
| **H-02** LED en PA2 = UART2_TX | F3 | ⬜ abierto — F1 confirma que UART2 es el puerto IAP (`mapa-memoria.md` §4) y aporta los 3 pines de LED de la referencia. Decisión en F3. |
| **H-03** FCC ID de serigrafía | F6 | 🔄 **AVANZADO**. DS §7 (pág. 20) declara **FCC `2BEK7LSM110A`** e **IC `32019-LSM110A`** (+ ANATEL `05243-24-12325`). El repo tiene grants a nombre de `2AS8LLSM110A` (titular anterior, SJI→SJIT). Sigue haciendo falta confirmación de GREATECH/SJI sobre qué ID aplica al lote. **Usar etiqueta, no serigrafía**, hasta tener respuesta. |
| **H-09** falta PSR coating | F6 | ⬜ abierto — DS §5.4 (pág. 17). F1 añade contexto: el README del footprint empuja al error **opuesto** (pad central con vías térmicas bajo el módulo). Ver `validacion-footprint.md` §4. |
| **H-06** `HAL_GetTick()` se congela en Stop2 | firmware | ⬜ abierto |
| **H-07** falta tope diario 130 msg + heartbeat 24 h | firmware | ⬜ abierto |
| **H-14** `DT-009` dice IWDG 4 s | firmware | ⬜ abierto |
| **H-15** `Hardware/Schematic/` y `Hardware/PCB/` vacíos | F3, F6 | ⬜ abierto |

---

## Hallazgos **nuevos** de F1 (no estaban en el Apéndice B)

| ID | Hallazgo | Fase | Severidad |
|---|---|---|---|
| **N-01** | El README del footprint dice que hay **pad central de GND con vías térmicas**. No existe (3 fuentes coinciden), y seguir esa instrucción mete cobre bajo el módulo — justo lo contrario del PSR coating que pide el DS §5.4. | F1 / F6 | **media** — induce a un error de layout |
| **N-02** | El **Apéndice A de la guía tiene dos errores** en el rendimiento de la antena: el RL a 921.6 MHz es **≈ −31.7 dB**, no −37.69 (mala lectura de un dígito en un bitmap de baja resolución), y la resonancia de la referencia está en **≈ 924–928 MHz**, no en 921.6 (el VSWR sigue bajando hasta 928). Reconciliado cruzando las gráficas de RL y VSWR. | F1 / F5 | baja — no cambia decisiones, pero era «dato verificado» |
| **N-03** | El **User Manual se contradice a sí mismo**: §1.2 (pág. 4) dice que el USB-serial del EVB es `FT2232HL/FTDI`; el esquemático de la hoja 3/3 (pág. 6) muestra `U2 = CP2104` (Silicon Labs). Manda el esquemático. Afecta al driver necesario para el rescate por IAP. | F7 | baja — media hora de diagnóstico perdida |
| **N-04** | El paso **`AT$RFS` tras flashear no es opcional**: sin él *«using updated RC lastly is not available»* (FW Download Guide, **pág. 6**; se repite en las págs. 10 y 12). Modo de fallo **silencioso** — el módulo parece funcionar y no transmite en la banda correcta. | F7 | **alta** — bloquearía el criterio de cierre de v0 |
| **N-05** | El área `0x08030000`–`0x08039FFF` (**40 kB**) no aparece en el mapa de memoria del UM §2. Tratar como reservada. | firmware | baja |
| **N-06** | La corriente de TX **solo está publicada a +21 dBm** (123 mA). No hay dato a +14 dBm, y **no se puede escalar** (la eficiencia del PA no es lineal y el consumo digital es fijo). Hay que medirlo. | F2 | media |
| **N-07** | El pad 2 del footprint está **+0.016 mm** fuera de retícula y el trazo superior de serigrafía **+0.082 mm** alto. Ambos sin efecto funcional; corregir cuando se toque el archivo. | F6 | nula |

---

## Cómo arrancar la siguiente sesión

**Siguiente: F3 (esquemático en KiCad).** El prompt de arranque completo y adaptado a KiCad está en `QUE-HACER-AHORA.md` §Paso 3, junto con las 6 decisiones que F3 va a pedir.

F2 queda para cuando lleguen las CR2450. No bloquea.

Dos correcciones al prompt de arranque, aprendidas en esta sesión:

1. La guía dice que la Tabla 5-1-1 «solo devuelve el título» con `pdftotext`. **No es cierto para esa tabla:** `pdftotext -f 14 -l 16 -layout` devuelve las 34 filas completas. Lo que sí es imagen y obliga a renderizar: **los esquemáticos (UM págs. 5 y 6), el dibujo de la antena (pág. 8), las gráficas de RL/VSWR (pág. 9), la Fig. 5-1-1, la Fig. 5-3-1 y la Fig. 6-1-1**. Usar `pdftotext` primero siempre: si devuelve datos, ahorra el render.
2. Los esquemáticos del UM están **girados 90°**. Hay que rotarlos (`PIL.Image.rotate(90, expand=True)`) antes de leerlos, y renderizarlos a **500 dpi** para que los valores de componente sean legibles.
