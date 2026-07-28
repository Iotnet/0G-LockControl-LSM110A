# GATE 2 — Fuente variable y emulación de CR2450 (issue #6)

> Guía de equipo y setup para medir el pulso TX con soporte del cap de 470 µF.
> Complementa el checklist del [issue #6](https://github.com/jdiaznxt/0G-LockControl-LSM110A/issues/6).
> El reporte final GO/NO-GO va en `REPORT.md` en esta misma carpeta.
>
> ⚠️ **Si al alimentar sin USB el LED azul parpadea 6 veces y no transmite**, no es
> la alimentación: es el rate limit del firmware. Causa y desbloqueo en
> [`TROUBLESHOOTING.md`](TROUBLESHOOTING.md).

## 1. Por qué no sirve alimentar por USB / puerto serial

Hallazgo correcto: **con el Nucleo alimentado por el USB del ST-LINK la prueba del
GATE 2 es inválida.** Tres razones:

1. **Es una fuente "dura".** El riel 5 V USB → regulador 3.3 V del Nucleo tiene
   impedancia de salida de miliohms: entrega el pico de TX sin caída de voltaje.
   El cap de 470 µF nunca se descarga y no hay dip que medir. Lo que el GATE 2
   mide es justamente el efecto de la **resistencia interna (IR ~10–40 Ω) de la
   CR2450**, que el USB no tiene.
2. **No se puede variar el voltaje** para reproducir batería fresca / 50% / EOL.
3. **Lazo de tierra.** El USB une el GND de la placa con el GND de la PC, y el
   osciloscopio une su GND a tierra física → ruido y offsets en una medición de
   decenas/cientos de mV.

## 2. Requisitos de la fuente (esto pesa más que la marca)

| Requisito | Valor | Por qué |
|---|---|---|
| Topología | **Lineal** (no conmutada) | Ripple <1–2 mVrms; una conmutada mete spikes del orden del dip que buscamos |
| Resolución | 10 mV / 1 mA | Barrer 1.8–3.3 V con precisión |
| Límite de corriente (CC) | Ajustable, usar ~0.3–0.5 A | Solo como protección — ver §4 |
| Corriente máxima | ≥1 A (sobra) | Pico TX ~50 mA @+14 dBm; ~123 mA si se prueba +22 dBm |
| Salida | Flotante (aislada de tierra) | Evita lazos con osciloscopio y ST-LINK |
| Deseable | Programable USB/RS-232 (SCPI) | Automatizar barridos de voltaje y logging |

## 3. Modelos recomendados (disponibilidad México, jul-2026)

| Opción | Modelo | Specs | Precio aprox. | Dónde |
|---|---|---|---|---|
| **Recomendada (calidad/precio)** | **Korad KA3005D / KD3005D** | Lineal 30 V/5 A, 10 mV/1 mA | ~$2,500–5,500 MXN según vendedor | Mercado Libre MX, Amazon MX, DigiKey MX |
| Variante programable | Korad KA3005**P** / KD3005**P** | + USB/RS-232 SCPI | +$500–800 MXN aprox. | Ídem |
| Paso arriba | **Rigol DP711** | Lineal 30 V/5 A, SCPI RS-232, transitorio <50 µs, OVP/OCP | ~$7,500 MXN | Amazon MX, FinalTest, ACMax |

Notas:
- Tenma 72-2540 y Velleman LABPS3005D son **rebrands de la misma Korad** — si
  aparecen más baratas, sirven igual.
- La DP711 se justifica si después se automatiza la validación low-power (M4)
  y el bring-up de la PCB (M6); para el GATE 2 solo, la Korad basta.
- **Evitar:** módulos buck tipo DPS3005/RD6006 y eliminadores conmutados — su
  ripple/spikes de conmutación (decenas de mV) contaminan la medición del dip.
- **No hace falta** un emulador de batería dedicado (Keysight E36731A, ITECH
  IT6412 con resistencia de salida programable, >$40k MXN): la resistencia
  serie de §4 logra lo mismo para este gate.

## 4. Punto crítico: la fuente sola TAMPOCO muestra el dip

Una fuente de banco también es una fuente dura. Conectada directa al módulo se
comporta igual que el USB: cero caída, prueba inválida. **Hay que emular la
resistencia interna de la CR2450 con una resistencia serie Rs:**

```
Fuente (V_oc) ──── Rs ────┬──── VDD módulo / 3V3 Nucleo
                          │
                       470 µF          ← el cap bajo prueba, después de Rs
                          │
GND fuente ───────────────┴──── GND placa ──── GND punta osciloscopio
```

| Estado CR2450 emulado | V fuente (circuito abierto) | Rs serie |
|---|---|---|
| Fresca | 3.0–3.2 V | 10–15 Ω |
| ~50% descarga | 2.8–2.9 V | 22–33 Ω |
| EOL | 2.0–2.2 V | 39–47 Ω |

- Rs: resistencias de **1–2 W** (a 100 mA sobre 47 Ω son ~0.5 W). Valores E12
  útiles: 10, 22, 33, 47 Ω. Cables cortos, todo en la protoboard del setup.
- **El límite CC de la fuente se deja ARRIBA del pico esperado (0.3–0.5 A)**
  para que nunca entre en modo CC: quien limita es Rs. Si la fuente entra en
  CC, su respuesta transitoria distorsiona la forma del dip.
- La punta del osciloscopio va en VDD **después de Rs** (sobre el cap), tierra
  de la punta lo más corta posible (spring/tip ground, no el caimán largo).
- Bonus perfil de corriente: shunt de 1 Ω entre Rs y el cap, canal 2 del
  osciloscopio midiendo su caída → I = V/1 Ω.

Con este arreglo además se puede **encontrar el umbral real de brown-out** de
forma reproducible: bajar V de la fuente de 100 en 100 mV hasta que el módulo
resetee durante TX, algo imposible de hacer de forma controlada con pilas.

## 5. Procedimiento con el Nucleo-WL55JC (sin serial)

Aprovecha el demo de 3 botones (`Acondicionamiento .../Programa_3_botones/`):
la TX se dispara por push-button, así que el USB no hace falta durante la medición.

1. **Flashear por USB/ST-LINK** el programa de botones. Verificar TX normal.
2. **Desconectar el USB por completo.**
3. Alimentar desde fuente + Rs + cap 470 µF por una de dos vías (UM2592):
   - **Pin 3V3** (CN6/CN7): alimenta el riel 3.3 V bypasseando el regulador.
     Simple, pero las cargas extra de la placa suman caída constante en Rs.
   - **Jumper IDD (JP2)**: retirar el jumper y alimentar el lado MCU →
     alimenta solo el dominio del STM32WL55. Medición más limpia (preferida).
4. Osciloscopio en VDD, trigger por flanco de bajada (nivel ~50–100 mV bajo el
   V de reposo), o trigger por GPIO si el firmware saca un pulso pre-TX.
5. Disparar TX con B1/B2/B3. En RC2 `SIGFOX_API_send_frame` bloquea **7–9 s y
   manda 3 frames** → capturar los 3 dips en una sola ventana (2 s/div) y
   luego hacer zoom al peor. El VDD_min del peor frame es el dato del gate.
   - **Poner `BTN_TX_REPLICAS` en 3** (el demo viene en 1). Con 1 réplica se ve
     un solo dip y se **subestima** el peor caso: con 3 frames el cap arranca el
     3.º ya parcialmente descargado, y ése es el `Vmin` que decide el gate. Las
     3 réplicas son 1 solo mensaje para el contrato Sigfox — no cuesta tokens
     extra, sólo energía.
6. Confirmación de recepción: backend Sigfox (flujo ya validado en issue #2).
7. Repetir la matriz fresca/50%/EOL de §4 y registrar VDD_min de cada caso.

## 6. Lo que la fuente NO reemplaza

El veredicto GO/NO-GO del issue #6 pide **CR2450 reales** (fresca, 50%, EOL).
La fuente sirve para caracterizar el circuito de forma reproducible, dimensionar
el cap (comparar electrolítico genérico vs polímero low-ESR) y encontrar el
umbral de colapso **antes** de quemar pilas; la corrida final del reporte se
hace con las 3 pilas físicas y se documenta en `REPORT.md`.
