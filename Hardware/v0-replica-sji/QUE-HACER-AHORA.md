# Qué hacer ahora — Franco

Estado: **F0 y F1 cerradas.** Tú eres las manos. Esto es la lista, en orden.

---

## Paso 1 — Commitear F1 · ~10 min · **hazlo antes que nada**

Abre `COMMIT-F1.md` y ejecuta los pasos 1–5 con Claude Code.

Sin esto, F1 no existe fuera de tu carpeta local. `ESTADO.md` es el hilo entre sesiones:
si no está en el repo, la sesión de F3 no sabe dónde te quedaste.

Después, el **paso 6** del mismo archivo: 5 correcciones al repo (el pinout viejo con 4 pines
mal, el README del footprint, `STM32WL55`→`STM32WLE5CC`, `TX_REPEATS`). Commit aparte para
que el PR de F1 quede limpio.

---

## Paso 2 — Instalar la librería KiCad · ~5 min

`kicad-lib/README.md` tiene las rutas exactas. Dos librerías de proyecto:
el símbolo (`LSM110A.kicad_sym`) y el footprint (`LSM110A.kicad_mod`).

**Comprueba dos cosas al abrirlo**, que son las que delatan un archivo mal generado:

1. El símbolo tiene **34 pines** y aparecen **7 pines GND** (1, 10, 12, 20, 23, 32, 34).
2. El footprint tiene **34 pads** y **ningún pad central**.

Si las dos cuadran, está bien.

---

## Paso 3 — Arrancar F3 (esquemático) en un chat nuevo

**No hace falta ningún chat intermedio.** Commiteas y arrancas.

Prompt de arranque (guía §7), ya adaptado:

```
Vamos a trabajar en la fase F3 (esquematico) de la replica v0 del diseño de
referencia SJI para el proyecto 0G LockControl (modulo LSM110A, Sigfox RC2).

IMPORTANTE: uso KiCad, no EasyEDA. La guia esta escrita mezclada; manda KiCad.
Ya existe libreria KiCad nativa en Hardware/v0-replica-sji/kicad-lib/
(simbolo de 34 pines + footprint LGA-34), generada y verificada en F1.

Antes de hacer nada:
1. Clona mi repo: git clone --depth 1 https://github.com/Iotnet/0G-LockControl-LSM110A.git
2. Clona el del fabricante (SOLO git clone, curl a raw.githubusercontent falla):
   git clone --depth 1 --filter=blob:none --sparse https://github.com/Support-SJI/LSM110A.git
   cd LSM110A && git sparse-checkout set --no-cone '/Document/*' && git checkout HEAD
3. Lee Hardware/v0-replica-sji/ESTADO.md primero: tiene las decisiones D-01..D-08,
   las preguntas abiertas de F3, la deuda declarada y la auditoria de F1.
   OJO con la numeracion: las decisiones de diseno viven en docs/decisiones-tecnicas.md
   con la serie DT- (D-04..D-08 = DT-012..DT-016). Esa serie es la que manda, y las
   decisiones nuevas de F3 se numeran DT- desde el principio, no D-.
4. Lee los 6 .md de Hardware/v0-replica-sji/00-fuente-de-verdad/. Son la fuente
   de verdad de F1: cada dato con documento y pagina. NO los re-derives.
5. Dime en que punto estamos, hazme las 6 preguntas obligatorias de F3, y espera mi OK.

Nota de metodo aprendida en F1: usa pdftotext PRIMERO (la Tabla 5-1-1 SI sale como
texto, la guia se equivoca en eso). Solo renderiza con pdftoppm lo que es imagen:
esquematicos del UM (pags. 5 y 6, GIRADOS 90 grados, rotar con PIL antes de leer,
500 dpi), dibujo de la antena (pag. 8, 600 dpi), graficas RL/VSWR (pag. 9),
Fig. 5-1-1, Fig. 5-3-1 y Fig. 6-1-1 del datasheet.

Al cerrar: actualiza ESTADO.md y preparame COMMIT-F3.md.
```

### Las 6 decisiones que F3 te va a pedir

Están en `ESTADO.md` con el argumento completo. Resumen para que las pienses antes:

| Decisión | Recomendación de F1 | Por qué |
|---|---|---|
| **Supervisor de reset 1.8 V** | **poblar** | Con pila, caer por debajo de 1.8 V puede corromper flash — y ahí viven las credenciales Sigfox, que no se recuperan. El EVB no lo trae porque va por USB. `limites-electricos.md` §5.1 |
| **Pull-ups I2C: 4.7 k o 10 k** | tú eliges, pero **alinea repo y BOM** | Ahora se contradicen. Da igual cuál, importa que sea uno |
| **Variante del DRV5032** | **push-pull (`FB`)** | Elimina el pull-up de PA1. Un pull-up de 10 k en bajo son **300 µA = 60× el módulo entero** |
| **u.FL en v0** | **sí, con selector de 0 Ω** | La referencia deja antena y SMA en paralelo permanente y los separa cortando pista. Eso no es un selector, es un divisor |
| **LED de debug** | sí, en **PA8 (24), PA11 (5) o PA15 (9)** | **Nunca en PA2**: es UART2_TX, tu única vía de rescate por IAP |
| **Red RF: ¿EVB o DS §6.1?** | poblar los footprints de **las dos**, montar la del EVB | La del EVB está certificada por el exhibit FCC; la del DS tiene protección ESD (y el DS declara solo ±2 kV). 5 huecos de 0402: cuesta área, no dinero |

---

## Lo que NO estás haciendo todavía, y por qué

**F2 (energía) queda pendiente** — es la fase que la guía recomienda hacer después de F1
(§6: «la que más riesgo elimina por hora invertida»), pero es **trabajo de banco, no CAD**.

Tienes osciloscopio, multímetro y el LSM. Te falta lo de la lista de F2:
**CR2450 reales** (la prueba exige pila real, no fuente de banco — el punto es medir el ESR)
y **una pila descargada a ~2.4 V** para el caso peor.

**No bloquea F3.** El diagrama de dependencias de la guía §6 pone F2 en paralelo. Pero sí
hay acoplamiento en un punto: **F2 puede forzar cambios en F3** (el valor y la tecnología
del condensador de soporte, y si hace falta el pivot a CR2477). Por eso F3 debe dejar ese
footprint dimensionado con holgura y no cerrar la BOM de alimentación hasta tener GATE 2.

**Sugerencia:** pide las pilas hoy. Cuando lleguen, F2 se hace en una tarde y con F3 ya
dibujado tendrás el esquemático listo para absorber el resultado.

---

## Orden completo hasta el uplink

```
[ HECHO ] F0  setup
[ HECHO ] F1  fuente de verdad          ← 6 .md + librería KiCad
   ↓
  Paso 1   commit + correcciones del repo        ← tú, ahora
  Paso 2   instalar librería KiCad               ← tú, ahora
   ↓
[ AHORA ] F3  esquemático (KiCad)        ← chat nuevo
           F4  BOM (puede ir en el mismo chat que F3)
   ↓
           F2  energía + GATE 2           ← cuando lleguen las pilas
           F5  antena                     ← ojo con B-01 y B-02
   ↓
           F6  layout
   ↓
           F7  fabricación y bring-up     ← el uplink cierra v0
```

**Una fase por chat.** Cada una lee `ESTADO.md`, trabaja, actualiza `ESTADO.md` y te deja
un `COMMIT-Fn.md`. El sandbox se borra al cerrar el chat y no importa: volver a clonar
tarda 10 segundos y lo que persiste son los archivos del repo.

---

## Dos cosas que no se te pueden olvidar

**1. Nunca «Full chip erase» en el módulo.** Borra el ID y el PAC de fábrica, no se
recuperan, y el módulo queda inútil para Sigfox de forma permanente. Es la única operación
de este proyecto que destruye hardware con un clic, y es el botón grande por defecto de
CubeProgrammer. Borrado **por sectores**, solo `0x08002000`–`0x0802FFFF`.
Detalle en `00-fuente-de-verdad/mapa-memoria.md` §3.

**2. `AT$RFS` después de flashear no es opcional.** Sin él, el RC configurado no queda
disponible: el módulo *parece* funcionar y no transmite en RC2. Fallo silencioso que
bloquearía el criterio de cierre de v0. FW Download Guide, pág. 6. Es **N-04**.
