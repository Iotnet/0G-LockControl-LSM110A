# PDFs fuente — no se commitean

Los `.md` de esta carpeta citan los PDFs del fabricante como `pdfs/<archivo>.pdf`.
Esos archivos **no están en el repo**: pesan, y ya están publicados por el fabricante.
Esta carpeta existe para que esas rutas tengan un sitio y para decir cómo conseguirlos.

## Cómo obtenerlos

```bash
git clone --depth 1 --filter=blob:none --sparse https://github.com/Support-SJI/LSM110A.git
cd LSM110A && git sparse-checkout set --no-cone '/Document/*' && git checkout HEAD
```

`curl` a `raw.githubusercontent.com` falla; usar `git clone`.

## Revisiones exactas usadas en F1

Verificar el SHA-256 antes de dar por buena cualquier cita: si no coincide, la revisión
es otra y las páginas pueden haberse movido.

| Archivo | Revisión | Págs. | SHA-256 |
|---|---|---|---|
| `DS_LSM110A_R08_241008.pdf` | R08 | 24 | `76abc36b9b1944b1144ae15f99bb6a38a378538c4d8f6b2f0175b722301f04b6` |
| `[SJIT]_LSM110A_UserManual_Rev1.4_240626.pdf` | Rev 1.4 | 33 | `02cb31b454a7e86d58b721f02134c4715715f181c5efb49bb288d4e4d6de6334` |
| `user-manual-antenna-trace-design.pdf` | — | 9 | `bf344d65f2a8c5b021d49e1ea858da03783482ddf5082b4c086eba9458e2e116` |

El tercero sí está en el repo: `Hardware/certificacion-FCC/user-manual-antenna-trace-design.pdf`.

```bash
sha256sum DS_LSM110A_R08_241008.pdf
```

## Método de extracción

`pdftotext -f 14 -l 16 -layout` devuelve la Tabla 5-1-1 completa como texto — probar
siempre eso primero. Solo hay que renderizar lo que es imagen: los esquemáticos del UM
(págs. 5 y 6, **girados 90°**, rotar antes de leer, 500 dpi), el dibujo de la antena
(pág. 8, 600 dpi), las gráficas de RL/VSWR (pág. 9), y las Fig. 5-1-1, 5-3-1 y 6-1-1
del datasheet.
