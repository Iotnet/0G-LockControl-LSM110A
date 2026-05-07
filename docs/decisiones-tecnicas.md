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
- **Referencia:** FCC ID: 2AS8LLSM110A — fccid.io/2AS8LLSM110A

## DT-011: Certificaciones — plan de cumplimiento
- **Fecha:** 2026-05-07
- **Contexto:** Para vender el dispositivo en México se necesitan: Sigfox Ready, NOM-208/IFT-008, y suscripción Sigfox.
- **Decisión:** Todas las certificaciones se tramitan post-MVP. El diseño cumple desde ahora las condiciones para no invalidar certificaciones heredadas.
- **Condiciones para preservar certificaciones:**
  1. No modificar la stack de Sigfox del SDK de SJI
  2. Usar antena PCB del diseño de referencia SJI
  3. No cambiar frecuencia de operación (RC2, 902-928 MHz)
  4. No exceder potencia máxima certificada (+22dBm)
  5. Etiquetar producto con "Contains FCC ID: 2AS8LLSM110A"
