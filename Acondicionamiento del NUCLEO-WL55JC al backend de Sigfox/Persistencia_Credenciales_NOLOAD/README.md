# Persistencia de credenciales Sigfox — Fix del linker con `(NOLOAD)`

Documentación técnica del **fix aplicado al linker script `STM32WL55JCIX_FLASH.ld`** para que las credenciales Sigfox (`ID`, `PAC`, `KEY`) escritas en la zona `0x0803E500` de la flash **no se borren cada vez que se re-flashea el firmware `Sigfox_AT_Slave`**.

> **TL;DR** — El SDK STM32CubeWL trae credenciales TEST hardcodeadas (`FEDCBA98` / ceros) que se mapean vía linker a la misma región donde después escribimos las credenciales reales del portal sfxp. Al flashear el `.elf`, esas credenciales TEST sobrescriben las reales. Modificando la sección del linker a `(NOLOAD)` el ELF ya no lleva datos para esa región y las credenciales reales sobreviven.

---

## Contenido

- [El problema](#el-problema)
- [Causa raíz técnica](#causa-raíz-técnica)
- [La solución: `(NOLOAD)`](#la-solución-noload)
- [Diff conceptual](#diff-conceptual)
- [Ruta del archivo modificado](#ruta-del-archivo-modificado)
- [Verificación post-fix](#verificación-post-fix)
- [Ciclo de trabajo antes vs después](#ciclo-de-trabajo-antes-vs-después)
- [Limitaciones y consideraciones](#limitaciones-y-consideraciones)

---

## El problema

Durante el desarrollo iterativo del firmware `Sigfox_AT_Slave` (por ejemplo, al agregar el módulo `buttons_app.c` documentado en la carpeta hermana [`../Programa_3_botones/`](../Programa_3_botones/)), cada vez que se re-compilaba y se re-flasheaba el firmware con **CubeProgrammer**, ocurría lo siguiente:

1. El firmware nuevo se cargaba correctamente en `0x08000000`.
2. Al hacer reset y consultar `AT$ID` vía Serial Monitor:
   ```
   AT$ID
   > FEDCBA98              ← credenciales TEST del SDK, NO las reales del device
   OK
   AT$PAC
   > 0000000000000000       ← ceros, NO el PAC real
   OK
   ```
3. Como consecuencia, cualquier uplink era rechazado por el backend Sigfox (autenticación AES-128 con la KEY equivocada).
4. Para restaurar la operación había que **volver a escribir manualmente** el archivo `sigfox_data_<ID>.bin` (obtenido del portal `my.st.com/sfxp`) en la dirección `0x0803E500` usando la sección **Sigfox Credentials → Write data** de CubeProgrammer.

Esta pérdida se repetía **en cada re-flasheo del firmware**, lo cual era un bloqueo para el ciclo de desarrollo normal (edit → compile → flash → test).

---

## Causa raíz técnica

Investigando con `pyelftools` sobre el ELF generado antes del fix, se identificó lo siguiente:

### 1. El SDK trae credenciales TEST hardcodeadas

El archivo `Sigfox/App/sigfox_data.h` del SDK define macros con valores dummy:

```c
#define SIGFOX_KEY   01, 23, 45, 67, 89, AB, CD, EF, 01, 23, 45, 67, 89, AB, CD, EF
#define SIGFOX_ID    FE, DC, BA, 98
#define SIGFOX_PAC   00, 00, 00, 00, 00, 00, 00, 00
```

Estos macros son usados en el runtime del stack Sigfox para inicializar una estructura `manuf_device_info_t` que se compila con `__attribute__((section(".USER_embedded_Keys")))`.

### 2. El linker mapea esa sección a la región de credenciales reales

El linker script `STM32WL55JCIX_FLASH.ld` original (antes del fix) contenía:

```ld
MEMORY {
  FLASH               (rx) : ORIGIN = 0x08000000, LENGTH = 248K
  USER_Key_region_ROM (rx) : ORIGIN = 0x0803E500, LENGTH = 768
}

SECTIONS {
  ...
  .USER_embedded_Keys : {
    . = ALIGN(8);
    *(.USER_embedded_Keys.encrypted_sigfox_data)
    *(.USER_embedded_Keys)
    . = ALIGN(8);
  } >USER_Key_region_ROM         ← se mapea a 0x0803E500
  ...
}
```

**La región `USER_Key_region_ROM` en `0x0803E500` es exactamente el mismo lugar** donde CubeProgrammer escribe el binario `sigfox_data_<ID>.bin` con las credenciales reales emitidas por ST desde el portal sfxp.

### 3. El ELF resultante lleva las credenciales TEST cargables

Al examinar con `pyelftools` (antes del fix) se observaba:

```
=== Segmentos LOAD del ELF ===
PT_LOAD paddr=0x08000000  filesz=76648  memsz=76648  <-- ESCRIBE firmware
PT_LOAD paddr=0x0803E500  filesz=48     memsz=48     <-- ESCRIBE credenciales TEST!
```

El segmento `PT_LOAD` en `0x0803E500` con `filesz=48` significa que el ELF **lleva 48 bytes reales** para grabar en esa dirección. Esos 48 bytes son las credenciales TEST del SDK.

### 4. CubeProgrammer respeta lo que dice el ELF

Al flashear un ELF, CubeProgrammer no interpreta el linker script — se guía por los **segmentos `PT_LOAD`** del ELF. Para cada segmento con `filesz > 0`, borra los sectores de flash correspondientes y escribe los datos. Resultado: el sector que contiene `0x0803E500` era borrado y sobrescrito con las credenciales TEST en cada re-flasheo.

**Resumen del ciclo destructivo:**

```
[cambio código C] → compilar → ELF nuevo lleva TEST creds embebidas
                                    │
                                    ▼
                              flashear ELF
                                    │
                    ┌───────────────┴───────────────┐
                    ▼                               ▼
      escribe firmware en 0x08000000      SOBRESCRIBE 0x0803E500
      (correcto, deseado)                  con TEST creds del SDK
                                                    │
                                                    ▼
                                      credenciales reales BORRADAS
                                                    │
                                                    ▼
                                        AT$ID devuelve FEDCBA98
```

---

## La solución: `(NOLOAD)`

El linker de GNU (`arm-none-eabi-ld`) soporta el atributo **`(NOLOAD)`** en las declaraciones de sección. Con `(NOLOAD)`:

- **Los símbolos se conservan.** La dirección `0x0803E500` sigue siendo referenciable por código con `extern sfx_u8 encrypted_sigfox_data[]`.
- **La sección se declara como `SHT_NOBITS`.** Es equivalente a `.bss` — existe en el mapa de memoria pero no contiene datos que cargar.
- **El segmento `PT_LOAD` correspondiente queda con `filesz=0`.** El ELF ya no lleva bytes para escribir en esa región.
- **CubeProgrammer no toca esa dirección al flashear.** El sector conserva su contenido previo.

**Efecto para nuestro caso:** una vez escrito el `sigfox_data_<ID>.bin` real con `Write data`, se preserva **indefinidamente** en cualquier re-flasheo del firmware, sin importar cuántos cambios en el código C hagamos.

---

## Diff conceptual

**Archivo:** `Projects/NUCLEO-WL55JC1/Applications/Sigfox/Sigfox_AT_Slave/STM32CubeIDE/STM32WL55JCIX_FLASH.ld`

**Cambio en la sección de sections (alrededor de la línea 138):**

```diff
   .fini_array (READONLY) : {
     ...
   } >FLASH

-  .USER_embedded_Keys : {
+  /* NOLOAD: mantiene las direcciones simbolicas en 0x0803E500 pero
+     NO carga datos durante el flasheo. Asi las credenciales reales
+     (sigfox_data_<ID>.bin escrito por CubeProgrammer Sigfox Credentials)
+     se preservan en cada re-flasheo del firmware.
+     Modificacion: Yahir Flores - 0G IoT Solutions, junio 2026 */
+  .USER_embedded_Keys (NOLOAD) : {
     . = ALIGN(8);
-    *(.USER_embedded_Keys.encrypted_sigfox_data) 
+    *(.USER_embedded_Keys.encrypted_sigfox_data)
     *(.USER_embedded_Keys)
     . = ALIGN(8);
   } >USER_Key_region_ROM

   /* Used by the startup to initialize data */
   _sidata = LOADADDR(.data);
```

**Cambio real:** una sola palabra `(NOLOAD)` insertada tras el nombre de la sección `.USER_embedded_Keys`. El resto del bloque queda idéntico. La versión completa del archivo modificado está en [`src/STM32WL55JCIX_FLASH.ld`](src/STM32WL55JCIX_FLASH.ld).

**Cambio a nivel bytes en el ELF resultante (verificado con `pyelftools`):**

| Atributo | ANTES (sin NOLOAD) | DESPUÉS (con NOLOAD) |
|---|---|---|
| `sh_type` de la sección | `SHT_PROGBITS` (con contenido) | `SHT_NOBITS` (sin contenido) |
| `sh_size` de la sección | 48 bytes | 48 bytes (igual, pero simbólicos) |
| `filesz` del `PT_LOAD` | 48 bytes | **0 bytes** |
| `memsz` del `PT_LOAD` | 48 bytes | 48 bytes (igual) |
| ¿CubeProgrammer escribe? | **SÍ** (borra + escribe) | **NO** (ignora la región) |

---

## Ruta del archivo modificado

**Ruta absoluta en el proyecto STM32:**

```
/Users/yflores/ST/STM32Cube_FW_WL_V1.5.0/Projects/NUCLEO-WL55JC1/Applications/Sigfox/Sigfox_AT_Slave/STM32CubeIDE/STM32WL55JCIX_FLASH.ld
```

**Ruta relativa desde la raíz del proyecto STM32CubeWL:**

```
Projects/NUCLEO-WL55JC1/Applications/Sigfox/Sigfox_AT_Slave/STM32CubeIDE/STM32WL55JCIX_FLASH.ld
```

**Copia archivada en este repositorio (versión completa post-fix):**

- [`src/STM32WL55JCIX_FLASH.ld`](src/STM32WL55JCIX_FLASH.ld)

> **Importante — no confundir el nombre:** el archivo se llama `STM32WL55JCIX_FLASH.ld`, correspondiente al part number del MCU (STM32WL55JC**I** con la letra final indicando encapsulado). No es `STMFLASH_Id` ni ningún otro nombre.

---

## Verificación post-fix

Después de aplicar el cambio y recompilar el proyecto, hay dos formas de confirmar que el fix funciona:

### 1. Inspección del ELF con `pyelftools` o `objdump`

Desde un Python con `pyelftools` instalado:

```python
from elftools.elf.elffile import ELFFile

with open('Sigfox_AT_Slave.elf', 'rb') as f:
    elf = ELFFile(f)
    for seg in elf.iter_segments():
        if seg['p_type'] == 'PT_LOAD':
            print(f"PT_LOAD paddr=0x{seg['p_paddr']:08X}  "
                  f"filesz={seg['p_filesz']}  memsz={seg['p_memsz']}")
```

**Salida esperada:**

```
PT_LOAD paddr=0x08000000  filesz=76648  memsz=76648
PT_LOAD paddr=0x08012B68  filesz=312    memsz=6264
PT_LOAD paddr=0x0803E500  filesz=0      memsz=48    ← filesz=0 confirma NOLOAD
PT_LOAD paddr=0x20001878  filesz=0      memsz=4608
```

La clave es el `filesz=0` del segmento en `0x0803E500`. Si sale `filesz=48`, el fix NO se aplicó (verificar el linker script y hacer Clean + Build).

Alternativa con `arm-none-eabi-objdump`:

```bash
arm-none-eabi-objdump -h Sigfox_AT_Slave.elf | grep -A 1 USER_embedded
```

Salida esperada:

```
.USER_embedded_Keys   00000030   0803e500   0803e500   ...   ALLOC
                                                              ^^^^^ solo ALLOC, NO LOAD
```

**Antes del fix** se leería `LOAD, ALLOC, READONLY` — con `LOAD` presente. **Después del fix** solo `ALLOC` (sin `LOAD`).

### 2. Prueba end-to-end en la placa

1. Escribir credenciales reales con CubeProgrammer → **Sigfox Credentials** → **Write data** → `sigfox_data_<ID>.bin` en `0x0803E500`.
2. Verificar con Serial Monitor:
   ```
   AT$ID    → <ID real, ej. 033E07FC>
   AT$PAC   → <PAC real, ej. B06EA49C9E0F11F4>
   ```
3. Modificar cualquier código C del proyecto (ej. cambiar un `printf` en `sgfx_app.c`).
4. Recompilar → generar ELF nuevo.
5. Flashear el ELF nuevo con **Erasing & Programming** (NO usar Full chip erase).
6. Reset físico con B4.
7. Volver a consultar `AT$ID`.

**Con el fix:** `AT$ID` sigue devolviendo el ID real. Credenciales preservadas.
**Sin el fix:** `AT$ID` devuelve `FEDCBA98`. Credenciales perdidas.

---

## Ciclo de trabajo antes vs después

**Antes del fix** — cada iteración requería 5 pasos manuales, uno propenso a error:

```
1. Editar código C
2. Build en CubeIDE
3. Flashear el .elf
4. Recargar credenciales (Sigfox Credentials → Write data)      ← FRICCIÓN
5. Reset + probar
```

**Después del fix** — cada iteración se reduce a 4 pasos, sin recargar credenciales nunca más:

```
1. Editar código C
2. Build en CubeIDE
3. Flashear el .elf                                              ← credenciales sobreviven
4. Reset + probar
```

La única vez que hay que escribir manualmente `sigfox_data_<ID>.bin` es **la primera vez** que se aprovisiona la placa (justo después de descargarlo del portal sfxp).

---

## Limitaciones y consideraciones

**Lo que el fix NO hace:**

- **No protege contra `Full chip erase`.** Si desde CubeProgrammer se hace **Erasing → Full chip erase**, todo el flash se borra incluyendo las credenciales. Este es el borrado nuclear y explícito — el fix solo protege contra el borrado accidental durante el flasheo normal del ELF.
- **No protege contra escritura manual en `0x0803E500`.** Si por error se escribe otra cosa en esa dirección con `Erasing & Programming → File path` apuntando a un binario cualquiera, se sobrescribe. El fix solo protege contra el borrado que provocaba el ELF del propio firmware.
- **No genera credenciales.** Sigue siendo necesario el flujo original: extraer chip certificate → subir a `my.st.com/sfxp` → descargar `sigfox_data_<ID>.bin` → escribir con `Write data`. Ver [`../Guia_completa.pdf`](../Guia_completa.pdf) fases 4-6.

**Reversibilidad:**

Para revertir el fix (por ejemplo, si se quiere restaurar el flujo original del SDK que sí carga credenciales TEST), basta con eliminar la palabra `(NOLOAD)` del linker script y recompilar. Ninguna otra parte del código depende de este cambio.

**Impacto en el tamaño del binario:**

Cero. Un `.elf` generado con NOLOAD tiene los mismos bytes de firmware que uno sin NOLOAD. Solo cambian los metadatos de sección/segmento del ELF (que no consumen flash).

**Compatibilidad con re-generación desde `.ioc`:**

El linker script `STM32WL55JCIX_FLASH.ld` **NO** se regenera automáticamente cuando se abre el `.ioc` en STM32CubeMX y se pulsa Generate Code. Solo se regenera si se toca explícitamente la configuración de Linker en CubeMX, lo cual es infrecuente. Por seguridad, se recomienda mantener este README actualizado como referencia — si en el futuro se hace una re-generación completa del proyecto y el linker script se sobreescribe, este documento es la evidencia de qué hay que volver a aplicar.

---

## Referencias

- **AN5480 Rev 8** — How to build a Sigfox application with STM32CubeWL (sección 5, Credenciales)
- **UM2609** — STM32CubeIDE user guide, capítulo "Modify the linker script"
- **GNU LD Manual** — Output section attributes → `(NOLOAD)`:
  https://sourceware.org/binutils/docs/ld/Output-Section-Type.html
- **ELF Specification** — section types `SHT_PROGBITS` vs `SHT_NOBITS`
- Guía completa del comisionamiento: [`../Guia_completa.pdf`](../Guia_completa.pdf)
- Fase donde se aprovisionan credenciales por primera vez: ver PDF, Fase 6

---

**Autor:** Yahir Flores — `yflores@iotnet.mx`
**Empresa:** 0G IoT Solutions (previamente WND México) — [0giotsolutions.com](https://0giotsolutions.com/)
**Versión:** 1.0 · Julio 2026
