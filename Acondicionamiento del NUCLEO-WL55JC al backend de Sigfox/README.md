# Acondicionamiento del NUCLEO-WL55JC al backend de Sigfox

Guía resumida del procedimiento de vinculación end-to-end de una placa **STMicroelectronics NUCLEO-WL55JC** (MB1389, MCU `STM32WL55JCI`) a la red Sigfox 0G bajo **Radio Configuration 2 (RC2 México)**, operada por **0G IoT Solutions** (previamente WND México). Procedimiento ejecutado y validado el **29 de mayo de 2026**.

El flujo cubre desde la instalación del entorno de desarrollo hasta el primer uplink confirmado en el backend de Sigfox, incluyendo los 10 errores reales encontrados durante el proceso y sus soluciones.

Empresa operadora: **0G IoT Solutions** — [0giotsolutions.com](https://0giotsolutions.com/)

---

# Contenido

- [Resultado obtenido](#resultado-obtenido)
- [Requisitos](#requisitos)
- [Mapa de las 8 fases](#mapa-de-las-8-fases)
- [Fase 1 · Instalar entorno](#fase-1--instalar-entorno)
- [Fase 2 · Conexión física](#fase-2--conexión-física)
- [Fase 3 · Compilar y flashear firmware](#fase-3--compilar-y-flashear-firmware)
- [Fase 4 · Extraer chip certificate](#fase-4--extraer-chip-certificate)
- [Fase 5 · Portal sfxp y credentials.zip](#fase-5--portal-sfxp-y-credentialszip)
- [Fase 6 · Escribir sigfox_data al flash](#fase-6--escribir-sigfox_data-al-flash)
- [Fase 7 · Registro en backend](#fase-7--registro-en-backend)
- [Fase 8 · Primer uplink](#fase-8--primer-uplink)
- [Errores frecuentes (los 10 reales encontrados)](#errores-frecuentes-los-10-reales-encontrados)
- [Direcciones de memoria críticas](#direcciones-de-memoria-críticas)
- [Comandos AT V1.5.0](#comandos-at-v150)
- [Documento completo](#documento-completo)
- [Referencias](#referencias)

---

# Resultado obtenido

| Variable | Valor |
|---|---|
| **Device ID Sigfox** | `033E07FC` |
| **PAC inicial (emitido por ST)** | `B06EA49C9E0F11F4` |
| **PAC post-registro (rotado)** | `BC07485F3FFF8B71` |
| **Device Type** | `Nucleo_WL` |
| **Group** | `DHW` |
| **Contract** | `UnaTag_test` |
| **Frecuencia uplink medida** | `902.1782 MHz` |
| **Base station receptora** | `6BCE` |
| **RSSI primer uplink** | `-127.00 dBm` |
| **Link Quality** | AVERAGE (3/5) |
| **Frames recibidos** | 1 de 3 |
| **Payload** | `48656C6C6F` ("Hello") |
| **Delay backend** | 3.2 s |
| **Versiones firmware** | APP V1.5.0 · MW_SIGFOX V1.8.0 · MW_RADIO V1.4.0 |

---

# Requisitos

**Hardware:**

- Placa NUCLEO-WL55JC (MB1389) con MCU `STM32WL55JCI`.
- Antena SMA compatible con banda 868/915 MHz (incluida en el kit).
- Cable USB micro-B.

**Software:**

- STM32CubeIDE 2.0.0+
- **STM32CubeProgrammer 2.6+** (crítico: el menú Sigfox Credentials existe desde 2.6).
- **STM32CubeWL V1.5.0** descargado de [st.com](https://www.st.com/en/embedded-software/stm32cubewl.html) — **NO** clonar desde GitHub (ver error #1).
- Cliente serial (Arduino Serial Monitor, `screen`, CoolTerm).

**Cuentas:**

- myST con acceso a `my.st.com/sfxp`.
- Admin en `backend.sigfox.com` con permisos en el grupo correspondiente.

---

# Mapa de las 8 fases

```
┌──────────────────────┐   ┌──────────────────────┐
│ 1 · Instalar entorno │ → │ 5 · Portal sfxp      │
└──────────┬───────────┘   └──────────┬───────────┘
           ↓                          ↓
┌──────────────────────┐   ┌──────────────────────┐
│ 2 · Conexión física  │   │ 6 · Escribir flash   │
└──────────┬───────────┘   └──────────┬───────────┘
           ↓                          ↓
┌──────────────────────┐   ┌──────────────────────┐
│ 3 · Flashear firmware│   │ 7 · Registro backend │
└──────────┬───────────┘   └──────────┬───────────┘
           ↓                          ↓
┌──────────────────────┐   ┌──────────────────────┐
│ 4 · Chip certificate │ ⮕ │ 8 · Primer uplink ✓  │
└──────────────────────┘   └──────────────────────┘
```

Las fases **4 y 5** son la extracción y validación de credenciales (etapa crítica). La fase **8** es la validación end-to-end del flujo completo.

---

# Fase 1 · Instalar entorno

```bash
# Crear carpeta limpia para el SDK
mkdir -p ~/ST
cd ~/Downloads
unzip stm32cubewl-v1-5-0.zip -d ~/ST/

# Verificar que SÍ trae la app Sigfox (a diferencia del GitHub público)
find ~/ST -type d -name "Sigfox_AT_Slave"
```

Salida esperada:

```
~/ST/STM32Cube_FW_WL_V1.5.0/Projects/NUCLEO-WL55JC1/Applications/Sigfox/Sigfox_AT_Slave
```

> El proyecto del SDK se llama `NUCLEO-WL55JC1` (banda alta 865–928 MHz), que es la apropiada para Sigfox RC2 México (902.2 MHz uplink).

---

# Fase 2 · Conexión física

1. Antena SMA → conector **CN12**.
2. Jumpers de fábrica: JP1 ON, JP3 ON, JP4 [1-2], JP7 ON.
3. USB micro-B → **CN1** (lado ST-LINK).
4. **LED5 verde (PWR)** encendido.

Verificación en macOS:

```bash
ls /dev/cu.usbmodem*                 # Debe enumerar al menos un puerto
ioreg -p IOUSB -l | grep -i stlink   # Debe mostrar STLINK-V3
```

En STM32CubeProgrammer:

| Parámetro | Valor |
|---|---|
| Port | SWD |
| **Mode** | **Under reset** ⚠️ |
| **Reset mode** | **Hardware reset** ⚠️ |
| Frequency | 12000 kHz |
| Speed | Reliable |

Click **Connect**. Target esperado: `STM32WLxx · Device ID 0x497 · Rev Y · 256 KB · Cortex-M4 · 3.27 V`.

---

# Fase 3 · Compilar y flashear firmware

**Importar proyecto en STM32CubeIDE:**

`File → Open Projects from File System` → ruta:

```
~/ST/STM32Cube_FW_WL_V1.5.0/Projects/NUCLEO-WL55JC1/Applications/Sigfox/Sigfox_AT_Slave/STM32CubeIDE/
```

**Build:** click derecho sobre `Sigfox_AT_Slave` → Clean → Build.

Resultado esperado:

```
text     data    bss    dec    hex   filename
75512    312     10544  86368  15160 Sigfox_AT_Slave.elf
Build Finished. 0 errors, 0 warnings.
```

**Flashear con CubeProgrammer** (NO usar `Run As` de CubeIDE — pide ST-Link Server, ver error #2):

1. Connect (Mode: Under reset).
2. Erasing & Programming → Browse → seleccionar `Sigfox_AT_Slave.elf`.
3. ✓ Verify programming · ✓ Run after programming.
4. **Start Programming**.

> Tras `Run`, aparece `Warning: Connection to device is lost`. **Es esperado**, el firmware Sigfox entra en low-power y suelta SWD (ver error #4).

**Verificación AT (Serial Monitor 9600 baud, Carriage return):**

Boot banner esperado:

```
APPLICATION_VERSION: V1.5.0
MW_SIGFOX_VERSION:   V1.8.0
MW_RADIO_VERSION:    V1.4.0
ATtention command interface
SIGFOX APPLICATION READY
```

Test:

```
AT             → OK
AT$ID          → FEDCBA98          (valor TEST, sin credenciales)
AT$PAC         → 0000000000000000  (valor TEST, sin credenciales)
```

> **Sintaxis V1.5.0:** comando sin `?` (`AT$ID`, no `AT$ID?`). El `?` muestra ayuda en lugar del valor.

---

# Fase 4 · Extraer chip certificate

1. Cerrar Serial Monitor (libera VCP/ST-LINK).
2. CubeProgrammer → Connect (Mode: Under reset).
3. Sidebar izquierdo → ícono **Sigfox Credentials**.
4. Pestaña **Read** → botón **Read** → aparecen 136 bytes en hex.
5. **Save chip certificate** → guardar como `chip_cert_<id_o_descripcion>.bin`.
6. **Copy chip certificate** → queda en clipboard.

Log esperado:

```
Size:    136 Bytes
Address: 0x1FFF3F04
Data read successfully
```

---

# Fase 5 · Portal sfxp y credentials.zip

1. Abrir [my.st.com/sfxp](https://my.st.com/sfxp) → login myST.
2. Pegar el chip certificate en el textarea (Cmd+V) o subir el `.bin`.
3. Click el botón **Sigfox™ credentials**.
4. Descarga automática de `sigfox_credentials.zip`.

Contenido del ZIP:

```bash
unzip -l sigfox_credentials.zip
  Length      Date    Time    Name
       48  ...                sigfox_data_033E07FC.bin   ← este flashearemos
     1607  ...                sigfox_data_033E07FC.h     ← header alternativo
```

> El sufijo del nombre **es el Device ID Sigfox**. En este caso `033E07FC`.

---

# Fase 6 · Escribir sigfox_data al flash

**Estructura del .bin (48 bytes):**

```bash
xxd sigfox_data_033E07FC.bin
00000000: fc07 3e03 b06e a49c 9e0f 11f4 0100 0000  ← ID LE + PAC + flags
00000010: 51ad 0eba 0e92 e90b 7fa6 78f1 bf23 3e5d  ← KEY AES-128 cifrada
00000020: 4d5f 3030 3438 5f41 4237 375f 3031 3fd5  ← chip serial ASCII + CRC
```

| Offset | Contenido |
|---|---|
| `0x00-0x03` | Device ID (little-endian) |
| `0x04-0x0B` | PAC |
| `0x0C-0x0F` | Flags |
| `0x10-0x1F` | KEY AES-128 cifrada |
| `0x20-0x2D` | Identificador interno ASCII |
| `0x2E-0x2F` | Checksum |

**Procedimiento en CubeProgrammer:**

1. Click ícono **Sigfox Credentials**.
2. Sección **SigFox credential provisioning** (abajo):
   - Configuration: `Binary-Raw`
   - **Address: `0x0803E500`** (autocompletado)
   - Binary file: Browse → `sigfox_data_033E07FC.bin`
   - Header file: vacío
3. Click **Write data** → mensaje `SigFox credential provisioning succeeded`.

**Verificación tras reset (botón B4):**

```
AT$ID    → 033E07FC          ✓ (ya no FEDCBA98)
AT$PAC   → B06EA49C9E0F11F4  ✓ (ya no ceros)
```

---

# Fase 7 · Registro en backend

URL: [backend.sigfox.com](https://backend.sigfox.com/auth/login)

**Caso A — Device pre-existente** (el más frecuente con placas de lab reutilizadas):

El device ya aparece en el backend con un PAC **diferente** al del `.h`. Esto es **normal**: el PAC se consume y rota al primer registro. La KEY AES-128 (que es lo que importa) no cambia y el device transmite correctamente.

**Caso B — Device nuevo:**

```
Menu Group       → New (si no existe)
Menu Device Type → New:
    Name: NUCLEO-WL55JC-RC2-DevTest
    Keep alive period: 0 (o 1440 min para 1/día)
    Payload display: Regular (raw)
    Downlink mode: Direct
Menu Device      → New:
    Device ID: 033E07FC
    PAC: B06EA49C9E0F11F4  ← se consumirá
    Device Type: el creado arriba
    Prototype: ☐ (producción real)
```

---

# Fase 8 · Primer uplink

Con la placa lista, Arduino Serial Monitor abierto (9600 baud, CR):

```
AT$RC=2                   → OK   (región RC2 México, persistida en EEPROM)
ATS302=22                 → OK   (potencia TX 22 dBm, máx FCC RC2)
AT$SF=48656C6C6F          → OK   (después de ~6 s)
```

`48656C6C6F` = "Hello" en hex. La placa transmite 3 réplicas (TX1/TX2/TX3) en frecuencias de hopping distintas.

**Verificar en backend:**

`Device 033E07FC → Messages`. En 2–30 segundos debe aparecer:

| Time | Data | Station | RSSI | Freq | Frames | LQI |
|---|---|---|---|---|---|---|
| 2026-05-29 11:41:05 | `48656c6c6f` | `6BCE` | `-127 dBm` | `902.1782 MHz` | 1 | AVERAGE |

---

# Errores frecuentes (los 10 reales encontrados)

| # | Síntoma | Solución express |
|---|---|---|
| **1** | GitHub clone de STM32CubeWL no trae carpeta `Sigfox/` | Descargar ZIP de [st.com](https://www.st.com/en/embedded-software/stm32cubewl.html) (incluye licencia UnaBiz). |
| **2** | CubeIDE 2.0.0 pide *ST-Link Server* al hacer Run | Flashear el `.elf` directamente con CubeProgrammer (acepta ELF). |
| **3** | Error `Unable to get core ID` / `No STM32 target found` | Mode: **Under reset** + Reset mode: **Hardware reset** en CubeProgrammer. |
| **4** | `Connection to device is lost` después de Run | **No es error** — firmware Sigfox entra en low-power y suelta SWD. Es esperado. |
| **5** | `AT$ID?` devuelve texto de ayuda en lugar del valor | Usar el comando **sin `?`** en V1.5.0: `AT$ID` (no `AT$ID?`). |
| **6** | No existe `rfconf.h` para definir `REGION_RC2` | En V1.5.0 la región es **runtime**, no compile-time. Usar `AT$RC=2`. |
| **7** | Workspace de CubeIDE muestra dos proyectos con Sigfox | Cerrar el SDK genérico (`LSM1x0A_SDK_LoRaWAN_Sigfox`), trabajar solo en `Sigfox_AT_Slave`. |
| **8** | PAC en backend ≠ PAC del `.h` | Normal: PAC rota al primer registro. KEY (lo que firma uplinks) no cambia. |
| **9** | Portal sfxp descarga ZIP de 0 bytes / Service unavailable | Cache, browser distinto, horario distinto. Inestabilidad documentada desde 2023. |
| **10** | Mass storage `NODE_WL55JC` no monta en macOS | No bloqueante — flasheamos por SWD, no por drag-and-drop. |

---

# Direcciones de memoria críticas

| Dirección | Tamaño | Región | Contenido |
|---|---|---|---|
| `0x08000000` | 256 KB | Flash | Firmware `Sigfox_AT_Slave_CM4` |
| `0x0803E500` | 2 KB | `USER_Key_region_ROM` | `sigfox_data.bin` (credenciales) |
| `0x1FFF3F04` | 136 B | System memory | Chip certificate (wafer probe) |
| `0x20000000` | 64 KB | SRAM | RAM de trabajo |

---

# Comandos AT V1.5.0

| Comando | Función |
|---|---|
| `AT` | Test (responde OK) |
| `AT$ID` | Lee Device ID (4 bytes hex) |
| `AT$PAC` | Lee PAC (8 bytes hex) |
| `AT$RC=N` | Configura región (N=1..7), persiste en EEPROM |
| `AT$SF=<hex>` | Envía uplink (≤ 12 bytes payload) |
| `AT$SF=<hex>,1` | Uplink con downlink request (consume 2 tokens) |
| `AT$SB=<bit>` | Envía 1 bit (mínimo overhead) |
| `ATS302=N` | Potencia TX en dBm (máx 22 para RC2 FCC) |
| `AT$RST` | Reset del MCU |

---

# Documento completo

La guía detallada de 29 páginas en LaTeX está en:

```
~/Documents/VS/Latex/Aprovisionamiento_Sigfox_RC2_NUCLEO-WL55JC2/
├── Guia_Sigfox_RC2_NUCLEO-WL55JC2.tex   ← fuente LaTeX
├── Guia_Sigfox_RC2_NUCLEO-WL55JC2.pdf   ← PDF compilado
└── img/                                 ← assets (logo 0G)
```

El PDF incluye portada, índice, las 8 fases con tablas y código, el diagrama TikZ del flujo, sección extendida de errores y soluciones, anexos técnicos (memory map, parámetros RF de RC2 México, comandos AT), referencias completas y resumen ejecutivo.

---

# Referencias

| Documento | URL |
|---|---|
| Producto NUCLEO-WL55JC | https://www.st.com/en/evaluation-tools/nucleo-wl55jc.html |
| UM2592 (HW manual MB1389) | https://www.st.com/resource/en/user_manual/um2592-stm32wl-nucleo64-board-mb1389-stmicroelectronics.pdf |
| AN5480 (Sigfox + CubeWL) | https://www.st.com/resource/en/application_note/an5480-how-to-build-a-sigfox-application-with-stm32cubewl-stmicroelectronics.pdf |
| STM32CubeWL (st.com, **NO GitHub**) | https://www.st.com/en/embedded-software/stm32cubewl.html |
| Portal credenciales sfxp | https://my.st.com/sfxp |
| Backend Sigfox | https://backend.sigfox.com/ |
| Sigfox API docs | https://support.sigfox.com/apidocs |
| Cobertura Sigfox México | https://coverage.sigfox.com |
| 0G IoT Solutions | https://0giotsolutions.com/ |

---

**Autor:** Yahir Flores — `yflores@iotnet.mx`
**Empresa:** 0G IoT Solutions (previamente WND México)
**Fecha:** 29 de mayo de 2026
**Versión:** 1.0
