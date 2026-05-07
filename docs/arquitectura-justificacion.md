# Justificación de arquitectura — Firmware API sin MCU externo

## 1. Decisión

El dispositivo 0G LockControl utiliza el módulo LSM110A programado directamente con firmware API (código de aplicación corriendo dentro del STM32WL integrado en el módulo). No se utiliza un microcontrolador externo enviando comandos AT por UART.

## 2. Contexto: dos modos de operación del LSM110A

El módulo LSM110A de Seongji (SJI) ofrece oficialmente dos variantes de firmware:

**Versión AT:** El módulo viene con firmware preinstalado que responde comandos AT por UART (ej: `AT+SEND=0,AABBCCDD`). Se requiere un MCU externo (host) que envíe estos comandos, lea sensores, y controle la lógica de aplicación. El módulo actúa como "modem" pasivo.

**Versión API:** El desarrollador programa directamente el STM32WL que está dentro del módulo usando el SDK proporcionado por SJI (disponible en github.com/Support-SJI/LSM110A). El código de aplicación convive con la stack de Sigfox/LoRa certificada. No se necesita MCU externo.

Ambas versiones son productos oficiales del fabricante SJI. La página del producto en Sigfox Partner Network indica explícitamente: "It is also available in API (application programming interface)-enabled variants that allow the integration of the customer's firmware inside the module as well as AT version."

Referencia: https://partners.sigfox.com/products/lsm110a

## 3. Por qué se eligió firmware API

### 3.1 Razones técnicas

- **Menor consumo de energía.** Un solo procesador en modo sleep (Stop2, ~1.8µA) en lugar de dos procesadores dormidos. Para un dispositivo alimentado por CR2450 que debe durar >2 años, cada microamperio cuenta.

- **Menor tamaño.** Sin MCU externo se eliminan: un chip (~QFN-32 o similar), su cristal, sus capacitores de desacoplo, y la traza UART. Esto permite cumplir el objetivo de tamaño <5cm.

- **Menor costo de BOM.** Se elimina un MCU ($1-3 USD), su cristal ($0.30), y ~5 componentes pasivos asociados ($0.20). Ahorro total: ~$2-4 USD por unidad.

- **Menor complejidad de firmware.** Con AT, se necesitan dos proyectos de firmware (host + módulo), protocolo de comunicación UART, manejo de timeouts, parsing de respuestas AT, y sincronización de estados entre dos procesadores. Con API, es un solo proyecto de firmware con acceso directo a periféricos.

- **Mejor control de bajo consumo.** Con API, el código de aplicación controla directamente cuándo entra y sale de Stop2, configura el RTC para wake-up, y maneja interrupciones EXTI sin latencia de comunicación UART.

### 3.2 Razones de producto

- El dispositivo es una alarma simple: leer dos sensores (acelerómetro I2C + magnético GPIO), transmitir por Sigfox, dormir. No hay procesamiento complejo que justifique un segundo procesador.

- El LIS2DW12 (acelerómetro) genera una interrupción de hardware por el pin INT1 que despierta directamente al STM32WL del módulo. Con MCU externo, esta interrupción despertaría al host, que luego tendría que despertar al módulo por UART — latencia y consumo innecesarios.

### 3.3 El "riesgo" de API y su mitigación

El riesgo principal de firmware API es que un bug en el código de aplicación puede colgar el procesador que también ejecuta la stack de radio. A diferencia de la versión AT, no hay un MCU separado que pueda resetear el módulo.

**Mitigación implementada:**

1. **Independent Watchdog (IWDG):** El STM32WL tiene un watchdog independiente con reloj propio (LSI). Se configura con timeout de 4 segundos. Si el código se cuelga, el MCU se resetea automáticamente y vuelve a operar.

2. **Separación de memoria:** El SDK de SJI utiliza un bootloader IAP. La stack de Sigfox y el bootloader están en una región protegida de flash. El código de aplicación está en otra región. Un bug en la aplicación no corrompe la stack certificada.

3. **No se modifica la capa de radio:** El código de aplicación solo llama funciones del SDK (`SigfoxSendFrame()` o equivalente). No se modifican parámetros de RF (frecuencia, potencia de modulación, timing del protocolo).

4. **Firmware simple:** La aplicación es ~300 líneas de código: configurar sensores, manejar interrupciones, armar payload de 12 bytes, llamar al SDK para transmitir, entrar en sleep. Superficie de error reducida.

## 4. Por qué esto NO invalida las certificaciones

### 4.1 Sigfox Verified (certificación del módulo)

La certificación Sigfox Verified del LSM110A cubre las especificaciones de RF y protocolo — es decir, que el radio transmite en la frecuencia correcta, con la potencia correcta, y sigue el protocolo Sigfox. Esta certificación aplica a la CAPA DE RADIO, no al código de aplicación que corre encima.

El firmware API usa exactamente la misma stack de Sigfox certificada que la versión AT. La diferencia es solo cómo se invoca: por comandos de texto UART (AT) o por llamadas a funciones C (API). La capa de radio es idéntica.

Prueba de esto: el propio fabricante SJI ofrece la versión API como producto oficial y mantiene el SDK público. Si la versión API invalidara la certificación, el fabricante no la ofrecería como producto certificado.

### 4.2 Sigfox Ready (certificación del producto final)

Para obtener Sigfox Ready para nuestro dispositivo, aplicamos el "enfoque modular" (modular approach). Según la documentación oficial de Sigfox Build:

"Modular approach: Only Radiated Performance tests are executed on the device. Evidence of compliance to RF & Protocol specifications are inherited from a Sigfox Verified modular design (module or ref. design)."

Referencia: https://build.sigfox.com/sigfox-ready-certification

Esto significa:
- Las pruebas de RF y protocolo se HEREDAN del certificado Sigfox Verified del LSM110A.
- Solo necesitamos realizar pruebas de rendimiento radiado (radiated performance) en nuestro producto final (PCB con antena y carcasa).
- No importa si usamos AT o API — la conformidad RF se hereda del módulo.

### 4.3 FCC (certificación de radiofrecuencia USA)

El LSM110A tiene certificación FCC (ID: 2AS8LLSM110A). Esta certificación cubre el hardware de radio del módulo. El código de aplicación que corre en el MCU no afecta la certificación FCC siempre que:
- No se modifiquen los parámetros de RF del radio (frecuencia, potencia máxima, modulación).
- Se use la antena especificada (traza PCB diseño SJI).

Nuestro firmware de aplicación no modifica ningún parámetro de RF. Solo llama funciones del SDK para transmitir datos.

Nuestro dispositivo debe etiquetarse con: "Contains FCC ID: 2AS8LLSM110A"

### 4.4 NOM-208 / IFT-008 (regulación México)

Esta certificación es para comercializar dispositivos de radio en México (banda ISM 902-928 MHz). Es independiente de Sigfox — aplica a cualquier transmisor en esta banda.

El LSM110A tiene FCC. México y Estados Unidos tienen un Acuerdo de Reconocimiento Mutuo (MRA), lo que permite aceptar reportes de laboratorio de FCC como base para la homologación mexicana. El proceso es:

1. Tomar el reporte FCC del LSM110A (disponible de SJI).
2. Contratar un agente regulatorio en México (NYCE, UL México, Bureau Veritas México).
3. Gestionar el certificado de homologación ante el IFT.
4. Etiquetar el producto con la información NOM requerida (en español).

Estado: PENDIENTE para post-MVP. No bloquea prototipado ni desarrollo.

## 5. Condiciones que deben mantenerse para preservar las certificaciones

| Condición | Razón | Estado |
|-----------|-------|--------|
| No modificar la stack de Sigfox del SDK de SJI | Preserva Sigfox Verified | ✅ Cumplida |
| Usar la antena PCB del diseño de referencia SJI | Preserva FCC y rendimiento radiado | ✅ En plan de diseño |
| No cambiar frecuencia de operación (902-928 MHz RC2) | Preserva FCC y Sigfox | ✅ Configurado en SDK |
| No exceder potencia máxima certificada (+22dBm) | Preserva FCC | ✅ Default +14dBm |
| Pasar prueba de rendimiento radiado (lab acreditado) | Obtener Sigfox Ready | ☐ Post-MVP |
| Tramitar homologación IFT | Obtener NOM-208 | ☐ Post-MVP |

## 6. Resumen ejecutivo

La arquitectura de firmware API sin MCU externo es:
- **Oficialmente soportada** por el fabricante del módulo (SJI).
- **No invalida** ninguna certificación existente (Sigfox Verified, FCC).
- **No impide** obtener certificaciones futuras (Sigfox Ready, NOM-208).
- **Técnicamente superior** para este caso de uso (menor consumo, tamaño y costo).
- **Mitigada contra riesgos** con watchdog independiente y separación de memoria.

La única condición es no modificar la capa de radio del SDK y usar la antena del diseño de referencia de SJI.
