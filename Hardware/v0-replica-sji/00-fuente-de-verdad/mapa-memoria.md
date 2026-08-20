# Mapa de memoria del LSM110A

**Fase:** F1 · **Fecha:** 2026-08-20
**Fuente:** UM Rev1.4 **§2 «Memory map» (pág. 13)** — la página es **texto**, extraíble con `pdftotext -f 13 -l 13`
**Complemento:** `[SJIT]_LSM1x0A_FW_Download_Guide_Rev1.5_240702.pdf` **§1.1** (págs. **4–6**)

La sección declara: **«LSM110A F/W version: V1.0.1»**. Todas las direcciones de abajo corresponden a esa versión de firmware.

---

## 1. Regiones (literal del UM §2, pág. 13)

| Región | Inicio | Fin | Tamaño | Contenido |
|---|---|---|---|---|
| **IAP (Bootloader)** | `0x08000000` | `0x08001FFF` | `0x2000` (8 192 B) | *«Area in IAP»* |
| **F/W** | `0x08002000` | `0x0802FFFF` | `0x2E000` (188 416 B) | *«Area in LSM110A F/W»* |
| *(sin documentar)* | `0x08030000` | `0x08039FFF` | `0xA000` (40 960 B) | — no aparece en el §2 |
| **LoRa user area** | `0x0803A000` | `0x0803BFFF` | `0x2000` (8 192 B) | *«Area in LoRa user data»* |
| **Sigfox user area** | `0x0803C000` | `0x0803DFFF` | `0x2000` (8 192 B) | *«Area in Sigfox user data»* |
| **Sigfox ID / PAC** | `0x0803E000` | *(no declarado)* | — | *«Area in Sigfox ID, PAC»* |

**Credenciales:** el manual precisa que *«The Sigfox ID/PAC (Credentials) is placed at `0x0803E500`»*.

### Dos huecos en la documentación — decirlo explícitamente

1. **`0x08030000`–`0x08039FFF` (40 kB) no está documentado.** El §2 salta del fin del F/W al inicio del área de usuario LoRa. No sabemos si es reserva, si lo usa el firmware internamente, o si simplemente no se documentó. **Tratar como reservado: no escribir ahí.**
2. **El fin del área Sigfox ID/PAC no está declarado.** Solo el inicio (`0x0803E000`) y la posición de las credenciales (`0x0803E500`).

**Nota de coherencia con H-13:** el mapa llega hasta `0x0803Exxx`, es decir un desplazamiento de ~250 kB desde `0x08000000`. Eso solo cabe en un dispositivo de **256 kB de flash**, que es lo que designa la `C` final de **STM32WLE5CC** (DS §1.1, pág. 4). El mapa **confirma indirectamente** el MCU y descarta las referencias del repo a otras variantes. Ver `pinout-34-pines.md` §4.

---

## 2. Mapa visual

```
0x08000000  ┌──────────────────────────────┐
            │  IAP bootloader      8 kB    │  ← rescate por UART2 (115200)
0x08002000  ├──────────────────────────────┤
            │                              │
            │  Firmware de aplicación      │  ← aquí va nuestro .bin
            │                    184 kB    │
            │                              │
0x08030000  ├──────────────────────────────┤
            │  ▒▒ SIN DOCUMENTAR  40 kB ▒▒ │  ← no escribir
0x0803A000  ├──────────────────────────────┤
            │  LoRa user area      8 kB    │
0x0803C000  ├──────────────────────────────┤
            │  Sigfox user area    8 kB    │  ← «must not be erased»
0x0803E000  ├──────────────────────────────┤
            │  Sigfox ID / PAC             │
0x0803E500  │    ►► CREDENCIALES ◄◄        │  ← NUNCA borrar
            └──────────────────────────────┘
```

---

## 3. Prohibiciones — avisos literales del manual

Las dos frases siguientes son citas exactas del UM §2 (pág. 13):

> **«The Sigfox area must not be erased and modified.»**

> **«※ Warning: Never erase the entire memory.**
> **Users are responsible for any problems caused by the erase.»**

### Qué significa esto en la práctica

**El LSM110A viene con las credenciales Sigfox de fábrica.** No hay reprovisión: a diferencia del flujo que se usó con la Nucleo-WL55JC, aquí el ID y el PAC ya están grabados en `0x0803E500` desde la fábrica.

**No hay recuperación.** Un «Full chip erase» borra el ID y el PAC. No se pueden regenerar, no los tiene SJI en un servidor, y el módulo queda inútil para Sigfox de forma permanente. Un módulo así solo sirve como pisapapeles o para LoRa. **Es la única operación de este proyecto que destruye hardware de forma irreversible con un solo clic.**

### Reglas operativas de obligado cumplimiento

| Herramienta | Prohibido | Hacer en su lugar |
|---|---|---|
| **STM32CubeProgrammer** | «Full chip erase» · «Erase all sectors» | Borrado **por sectores**, solo el rango `0x08002000`–`0x0802FFFF` |
| **ST-Link Utility / CLI** | `-e all` | `-e <sectores del rango de F/W>` |
| **Bootloader IAP** | — | Es el camino **seguro**: solo escribe el área de F/W |
| **Debugger (SWD)** | escribir por encima de `0x08030000` | nada |
| **Option bytes / RDP** | subir el nivel de RDP a 2 | irreversible, bloquea el SWD para siempre |

**La vía preferente para flashear v0 es el IAP por UART2**, no el borrado por SWD. El IAP no puede tocar el área Sigfox por construcción. El SWD sí puede, y por eso es el camino peligroso aunque sea el más cómodo.

> **Aviso para el checklist de F7:** el paso «flashear firmware» del bring-up tiene que decir **explícitamente** qué método de borrado usar. Si alguien abre CubeProgrammer y pulsa el botón grande, el módulo se pierde. Esto no es hipotético: es el comportamiento por defecto de la herramienta.

---

## 4. Secuencia de descarga por IAP (FW Download Guide §1.1, págs. 4–6)

Es la vía de rescate si el SWD no engancha, y el camino recomendado en general. Verificado literal, con la página exacta de cada paso:

| Paso | Detalle | Pág. |
|---|---|:---:|
| 1 | Terminal con **Ymodem** (ExtraPuTTY 0.30, o Tera Term **4.72** — *«tera term does not work in the latest version (4.106)»*) | 3–4 |
| 2 | Serie a **115200** baudios | **4** |
| 3 | **Pulsar reset e introducir `1` antes de 0.5 s** | **4** |
| 4 | Files Transfer → Ymodem → Send, seleccionar el `.bin` | 4 |
| 5 | Al terminar imprime el tamaño del binario y arranca el F/W | 5 |
| 6 | El firmware de usuario corre a **9600** baudios (≠ 115200 del bootloader) | **5** |
| 7 | **Tras configurar el modo Sigfox, ejecutar `AT$RFS`** | **6** |

> El mismo procedimiento se repite para Tera Term (págs. 7–10) y en el §2 de F/W merge (págs. 11–12); `AT$RFS` reaparece en las págs. **6, 10 y 12**. Las tres veces con la misma advertencia.

### El paso 7 no es opcional

Aviso literal del manual:

> *«If you don't setting 'Initializing of restore flash factory settings', using updated RC lastly is not available»* — FW Download Guide, pág. 6

Es decir: **sin `AT$RFS` después de flashear, el RC configurado (RC2 en nuestro caso) no queda disponible.** El módulo parecerá funcionar y no transmitirá en la banda correcta. Es un modo de fallo silencioso, y hay que ponerlo en el checklist de F7 como paso con casilla propia.

### El puerto del IAP es UART2 — verificado

**UM §1.3, hoja 3/3 (pág. 6)** muestra el USB-serial del EVB conectado así:

```
PA2 (UART2-TX) ──[ R6 22 Ω ]── RXD (pin 20) ┐
                                            ├── U2 = CP2104
PA3 (UART2-RX) ──[ R7 22 Ω ]── TXD (pin 21) ┘
```

Confirma que el puerto AT/IAP del módulo es **UART2 = PA2 (pin 14) / PA3 (pin 13)**, con resistencias de 22 Ω en serie.

**Por tanto: sacar UART2 a conector en v0 es obligatorio, no un extra de debug.** Es la única vía de rescate si el SWD falla, y es el camino que no puede borrar las credenciales. Y refuerza **H-02**: poner el LED en PA2 inutiliza el puerto de rescate.

> **Discrepancia documental encontrada:** el **UM §1.2 (pág. 4)** describe el USB-serial del EVB como *«USB to serial IC: FT2232HL/ FTDI»*, pero el **esquemático de la hoja 3/3 (pág. 6)** muestra `U2 = CP2104` (Silicon Labs). **El esquemático manda.** Afecta al driver que hay que instalar para usar el IAP: se necesita el **CP210x VCP de Silicon Labs**, no el de FTDI. Anotarlo en F7 — es media hora de diagnóstico perdida si alguien busca un puerto FTDI que no va a aparecer.

---

## 5. Registro de ID/PAC en el backend — pregunta obligatoria de F7

Las credenciales vienen de fábrica en el módulo, pero **estar grabadas no es estar registradas.** El ID y el PAC hay que darlos de alta en el backend de Sigfox (UnaBiz / WND México) antes de que un uplink aparezca en ninguna parte.

Los dos valores se leen sin tocar la flash, por API: `SIGFOX_API_get_device_id()` (4 bytes) y `SIGFOX_API_get_initial_pac()` (8 bytes) — `Sigfox API manual §2.1`, pág. 5. Es la forma correcta de obtenerlos: **leerlos, no escribirlos.**

**Pregunta para Franco (F7):** ¿el ID/PAC de los módulos del lote ya está registrado en el backend RC2? Si no, hay que hacerlo antes del bring-up — el criterio de cierre de v0 es un uplink **visible en el backend**, y sin registro previo el uplink se emite pero no aterriza en ningún sitio.
