# Sistema mínimo LSM110A — Prototipo v0 (PCB mínima)

**0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/
Julio 2026 · Autor: Yahir Flores · Cierra milestones **M1** (esquemático) y **M3** (layout+gerbers) del Plan MVP.

> Estado de partida: firmware M2 con reed+accel **ya funciona** (Y1–Y3 ✅, validado en NUCLEO-WL55JC con contacto magnético en PA1). BOM v0 verificada en LCSC. Footprint del módulo listo. **Lo único vacío es `Hardware/Schematic/` — esta guía lo cierra.**

---

## 1. Alcance del prototipo v0

Tarjeta mínima de ~35×45 mm con: LSM110A + CR2450 + sensor magnético + acelerómetro + SWD + antena. Sin display, sin downlink, sin carcasa. Objetivo: **uplink Sigfox RC2 real desde el módulo** con los mismos eventos de puerta ya validados en la Nucleo.

## 2. Esquemático — conexiones (net por net)

Fuente de verdad de pines: `Hardware/v0-replica-sji/00-fuente-de-verdad/pinout-34-pines.md` §1
(datasheet **R08**, Tabla 5-1-1). Referencias = `Hardware/BOM/bom-mvp-v0.csv`.

> **Corregido (H-01):** los números de pin de I2C e INT1 de esta sección venían del pinout
> viejo y estaban mal. Ya están actualizados abajo. **Queda un punto abierto: el LED de
> debug** — ver la nota en §2.4.

### 2.1 Alimentación

```
BAT1 (CR2450) ──┬── U4b 0Ω (default)  ──┬── VDD (pin 11)
                └── U4a TPS7A02 2.5V ───┘   (poblar solo UNO — decisión P3)
VDD ── C_TX 470µF tántalo ── GND      (pulso TX, decisión P2)
VDD ── C11 4.7µF ── GND               (bulk)
VDD ── C1 100nF ── GND                (pegado al pin 11)
GND: pines 1,10,12,20,23,32,34 → plano sólido
```

### 2.2 Sensor magnético (el corazón del producto)

```
U3 DRV5032 (SOT-23): VCC→VDD + C2 100nF · GND→GND · OUT (pin 2) → PA1 (pin 15 del módulo)
J3 bornera/JST 2P:   terminal para contacto reed EXTERNO, en paralelo: PA1 ↔ GND
R1 10k pull-up VDD→PA1 (respaldo del pull-up interno; DNP si molesta el consumo)
```

Ambos son activo-bajo con imán presente → **cero cambio de firmware** (`reed_switch.c` sirve para los dos). El J3 permite usar el mismo contacto de puerta ya probado en la Nucleo.

### 2.3 Acelerómetro

```
U2 LIS2DW12: VDD+VDDIO→VDD + C3/C4 100nF · SCL→PA9 (pin 3) · SDA→PA10 (pin 4)
R2,R3 pull-ups I2C → VDD (ver §3, discrepancia 4.7k vs 10k)
INT1 → PA0 (pin 16), pull-down interno en firmware
```

### 2.4 Debug / misc

```
J1 SWD 5p: VDD · SWDIO PA13 (pin 7) · SWCLK PA14 (pin 8) · NRST (pin 30) · GND
J2 UART 3p (footprint, NO poblar en producción): PB6 TX (pin 19) · PB7 RX (pin 18) · GND
LED1 verde + R8 1k ← PENDIENTE, ver nota (NO PA2)
NRST ── C5 100nF ── GND
BOOT0 (pin 31): flotante (pull-down interno) + testpoint TP1 (recovery por bootloader)
```

> **El LED no puede ir en PA2.** Este documento lo ponía en «PA2 (pin 16)», que estaba mal
> por las dos puntas: PA2 es el **pin 14**, y su función es `UART2_TX` — el puerto del
> bootloader IAP, la única vía de rescate si el firmware deja el módulo sin SWD. El pin 16
> es PA0, que ya lleva INT1 del acelerómetro.
>
> Pines libres donde el diseño de referencia de SJI pone sus LEDs: **PA8 (24), PA11 (5),
> PA15 (9)**. Ninguno choca con UART2. **La elección la cierra F3** (hallazgo H-02), así que
> aquí queda sin asignar a propósito.

### 2.5 RF

```
RF_OUT (pin 33) ── traza 50Ω ── ANT1 antena PCB (diseño de referencia SJI)
Opcional v0: footprint u.FL + 0Ω selector en la traza, para probar con whip
y comparar RSSI en el backend antes de congelar la antena PCB.
```

El módulo ya integra matching + filtro armónico + TCXO → **no lleva matching externo**, solo la traza a 50Ω.

## 3. Correcciones a la BOM v0 (hallazgos)

| # | Hallazgo | Acción |
|---|----------|--------|
| 1 | **U3 `DRV5032DU` NO es omnipolar** — datasheet TI tabla 5-1: DU = **unipolar (solo polo sur), push-pull, 20 Hz**. La BOM lo describe como omnipolar. Con DU, el imán del kit debe montarse con orientación controlada o no dispara. | Cambiar a **DRV5032FB** (omnipolar, push-pull, **5 Hz = ~3× menos consumo**, ideal puerta) o al sustituto ya listado **FCDBZR C527532** (omnipolar, open-drain 20 Hz, requiere el pull-up R1). **Mismo footprint SOT-23, mismo pin de salida** — cero cambio de layout. |
| 2 | Pull-ups I2C: `pinout-lsm110a.md` dice **4.7kΩ**, la BOM solo lista 10k. A 100 kHz Standard Mode con trazas cortas, 10k funciona; 4.7k da más margen. | Decidir uno y alinear ambos documentos (sugerencia: 10k de la BOM, bus corto). |
| 3 | La BOM no incluye conector para el sensor externo ni u.FL de pruebas. | Agregar J3 (JST-XH 2P o bornera 5.08 mm) y opcional u.FL (C2557956 o similar) + 0Ω. |
| 4 | Spec §6 dice "Esquemático KiCad"; la ruta real es **EasyEDA Pro → JLCPCB**. | Actualizar spec (decisión de hoy). |

## 4. Layout — reglas para 2 capas JLCPCB

- Stackup: FR4 **1.6 mm, 2 capas**, cobre 1 oz. Traza RF microstrip ≈ **2.8 mm** (ya calculado, nota en pinout doc). Si el ancho estorba, pasar a coplanar waveguide con gap y recalcular en la herramienta de impedancia de JLCPCB.
- **Plano GND sólido bajo el módulo y la traza RF** en bottom; sin trazas cruzando debajo.
- **Keep-out de la antena**: sin cobre (ni planos) bajo el área de antena en AMBAS capas; respeta el área del diseño de referencia SJI.
- Stitching vias a lo largo de la traza RF cada ~3 mm (λ/10 @ 915 MHz ≈ 3.3 cm → sobra con 3-5 mm).
- C1/C11/C_TX pegados al pin 11; el DRV5032 y su 100nF cerca del borde donde va el imán.
- CR2450 en bottom (holder MY-2450-01) para dejar top limpio para RF/sensores.
- Módulo LSM110A: half-through holes hacia el borde de la PCB si es posible (facilita soldado e inspección a mano).

## 5. Orden en JLCPCB

- PCB: 5 pzas, 2 capas, 1.6 mm, HASL sin plomo (suficiente para v0), verde.
- **SMT assembly económico (un lado)**: solo los Basic (R, C, LED, U4b) + Extended baratos (U2, U3, BAT1 holder, C_TX). Cada Extended agrega fee de setup — con la BOM v0 son ~4-5 fees, aceptable.
- **Hand-load (tú)**: U1 LSM110A (no está en LCSC — cautín + flux en los half-holes, 10 min), U4a si se prueba la config regulada, conectores.
- Pedir **stencil** solo si ensamblas todo a mano; con SMT de JLC no hace falta.

## 6. Bring-up post-fab (checklist)

1. Inspección visual + continuidad: VDD↔GND sin corto **antes** de poner la pila.
2. Con fuente de banco a 3.0 V limitada a 50 mA: corriente reposo (sin firmware ≈ decenas de µA).
3. ST-LINK al J1 → `STM32CubeProgrammer` debe detectar el **STM32WLE5** del módulo.
4. Flashear firmware (§7) → logs por J2 (UART debug).
5. Evento de puerta (imán) → LED + log + **uplink visible en backend Sigfox RC2**.
6. Comparar RSSI backend: antena PCB vs whip (si montaste el u.FL).
7. Medir consumo sleep con multímetro en serie (meta: <5 µA con pila directa).

## 7. Firmware (ya casi listo)

- Proyecto: `LSM1x0A_SDK_LoRaWAN_Sigfox` (SDK SJI, `~/GitHub/LSM110A`), con `reed_switch.c` + `app_sensors.c` ya integrados y disparando TX (Y1–Y3 ✅).
- Pendiente para el prototipo: **Y4** (test bench Nucleo documentado) y **F1–F3** de Franco (payload 12 B de spec §4.4 + `sigfox_send_alarm()`); mientras no estén, el TX actual ya sirve para validar RF de la tarjeta.
- El módulo es **STM32WLE5 (single-core)** — el SDK SJI ya lo contempla; flashear por SWD (J1) con el ST-LINK externo, o el de la Nucleo en modo "external target".
- Credenciales Sigfox del módulo: el LSM110A trae ID/PAC propios de fábrica (contrato UnaTag_test / registrar en backend con el PAC de SJI).

## 8. Ruta a prototipo (2-3 semanas)

| Días | Actividad |
|------|-----------|
| 1–3 | Esquemático en EasyEDA Pro (este doc §2) + ERC + revisar con Franco |
| 4–6 | Layout (§4) + DRC JLCPCB + gerbers → **pedir** (BOM §3 corregida) |
| 7–14 | Fab + envío a MX. En paralelo: Y4 en Nucleo y F1–F3 |
| 15–17 | Bring-up (§6) + flasheo + uplink real desde el módulo |
| 18+ | M6: integración FW final en PCB custom, prueba de campo |

---
*Referencias: `docs/pinout-lsm110a.md`, `docs/spec-producto.md` (decisiones P1–P8, payload §4.4), `Hardware/BOM/bom-mvp-v0.csv`, HANDOFF-FW-M2 §5, datasheet TI DRV5032 (tabla 5-1), manual integrator SJI (antena/certificación FCC 2AS8LLSM110A).*
