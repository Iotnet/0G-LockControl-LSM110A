# Red RF — topología, valores y traza de 50 Ω

**Fase:** F1 · **Fecha:** 2026-08-20
**Fuentes:** `user-manual-antenna-trace-design.pdf` (exhibit FCC, en nuestro repo) **págs. 5 y 6** · UM Rev1.4 **§1.3 hoja 2/3 (pág. 5)** y **§1.5 (pág. 8)** · DS R08 **§6.1 Fig. 6-1-1 (pág. 19)** y **§1.1 (pág. 4)**

---

## 1. Topología de la referencia (FCC pág. 6 + UM §1.3, pág. 5)

Las dos fuentes muestran **el mismo circuito** y coinciden en valores, encapsulados y orden. El esquemático del UM añade el conector SMA y la línea de corte, que el exhibit FCC no dibuja.

```
                                     ┌──────────── CON101  SMA_3P_ST  (pin 3)
                                     │             pines 1 y 2 → GND
                                     │
  ANT1 ─────────────────┬────────────┴──── L101 ────┬──────────── RFOUT (pin 33)
  PCB-pattern_Antenna   │            (serie)   0R/1005      │
  (pin 1)               │                                   │
                       C101                                C102
                    2.2 pF/1005                          DNI/1005
                     (POBLADO)                        (NO POBLADO)
                        │                                   │
                       GND                                 GND

              └─ dentro del recuadro «CUT» del UM ─┘
```

**Orden desde el módulo hacia la antena — este es el dato que importa:**

| # | Componente | Valor | Encapsulado | Conexión | ¿Poblado? |
|---|---|---|---|---|---|
| 1 | **C102** | DNI | 1005 (= **0402** imperial) | shunt a GND, **lado RFOUT** | ❌ no |
| 2 | **L101** | **0 Ω** | 1005 (0402) | **serie** en la línea | ✅ sí |
| 3 | **C101** | **2.2 pF** | 1005 (0402) | shunt a GND, **lado antena** | ✅ sí |

> **El 2.2 pF va del lado de la antena, no del lado del módulo.** Es el único componente reactivo poblado de toda la red; invertirlo con el hueco de C102 cambia la impedancia vista por el módulo. Confirmado en las dos fuentes independientes.

**Es una red en π con solo un elemento poblado.** El footprint de C102 y el de L101 existen para poder convertirla en una L o en una π real si la antena de nuestra placa desafina. **Los tres footprints deben poblarse en el layout de v0** aunque dos queden vacíos — es el margen de ajuste de F5/F7.

### La red no es «matching» en sentido estricto

`L101 = 0 Ω` significa que la referencia **no transforma impedancia**: el módulo ya sale a 50 Ω (`DS §1.1`, pág. 4: *«RF interface optimized to 50 Ω»*). C101 = 2.2 pF es un **ajuste fino de sintonía** de la IFA, no una red de transformación. Consecuencia para F5: si la antena de nuestra placa mide mal, lo que se toca primero es **C101**, no L101.

### El recuadro «CUT» (UM §1.3 pág. 5 + §1.9 pág. 12)

En el EVB, la antena PCB **y** el conector SMA cuelgan del **mismo nodo** (el de C101). El recuadro de línea discontinua rotulado `CUT` encierra `ANT1 + C101`: cortando ahí se aísla la antena PCB y el módulo queda radiando solo por el SMA. Es el mecanismo de «EVB Radiation → Conduction Change» del §1.9.

**Para v0 esto NO se replica tal cual.** F3 debe decidir el selector (0 Ω) entre antena PCB y u.FL en lugar de un corte de pista; ambos caminos en paralelo permanentemente es un divisor, no un selector.

---

## 2. Red de protección ESD / opción de filtro (DS §6.1, Fig. 6-1-1, pág. 19)

El **datasheet** propone, para un producto real, una red distinta de la del EVB:

```
  RFOUT (33) ──┤├── C3 100 pF ──┬──── C4 (NC) ──┬── R2 0 Ω ──┬── C5 (NC) ──── CON101 SMA
               (serie)          │               │  (serie)   │
                              L1 47 nH          │            │
                            (shunt a GND)      GND          GND
               └─ recuadro «Options for ESD.» ─┘
```

| Componente | Valor | Papel |
|---|---|---|
| **C3** | 100 pF, **serie** | bloqueo de DC |
| **L1** | 47 nH, **shunt a GND** | camino de descarga a tierra para ESD |
| C4, C5 | NC (footprint vacío) | huecos de ajuste |
| R2 | 0 Ω, serie | selector / punto de corte |

**C3 + L1 forman un paso-alto**: a 902 MHz, 100 pF ≈ 1.8 Ω en serie (transparente) y 47 nH ≈ 266 Ω en shunt (poca carga), pero a DC y baja frecuencia la energía de ESD se va a tierra por L1 en vez de entrar al pin 33.

Esto resuelve **H-10**. Nota de coste cero para la BOM: es lo mismo que ya hay que poblar en la red del EVB, reubicado — **no son componentes nuevos, es un orden distinto de los mismos footprints**. La decisión de qué red va en v0 (la del EVB o la del DS) es de **F3**, y hay tensión real:

- La guía dice «cuando dudes, replica». La red del EVB es la que está **certificada** por el exhibit FCC.
- La red del DS es la que el fabricante recomienda para producto, y la ESD de un dispositivo a pila con antena en el borde es un riesgo real que el EVB (alimentado por USB, en un banco) no tiene.

**Recomendación para F3:** poblar los footprints de **las dos** (son 5 huecos de 0402 en la misma línea) y poblar en v0 la topología del EVB —la certificada—, dejando C3/L1 sin poblar como opción de bring-up. Cuesta área, no dinero, y es reversible sin girar la placa.

---

## 3. Traza de 50 Ω (FCC pág. 5, «application PCB information»)

Literal del exhibit:

```
- PCB : 2-layer, 1.6mm
- Impedance line width : 1.0mm
- Clearance : 0.15mm
- FR4 PCB εr = 4.3
```

Tamaño de la placa de referencia: **50 × 80 mm** (acotado en la misma página).
La flecha roja de la pág. 5 rotula la traza corta entre el módulo y la red como `50 ohm matching parrern` [sic].

### Geometría → es **CPWG**, no microstrip

«Clearance 0.15 mm» junto a «line width 1.0 mm» sobre un plano de tierra continuo describe una **coplanar waveguide con tierra** (CPWG): tierra a los lados a 0.15 mm y tierra debajo a 1.6 mm. No es microstrip.

### Verificación numérica (R5 — calculado, no citado)

Modelo Wadell/Ghione con aproximación de Hilberg para K(k)/K(k′):

| w | s | h | εr | **Z₀** | ε_eff |
|---|---|---|---|---|---|
| 1.00 mm | 0.15 mm | **1.6 mm** | 4.3 | **51.22 Ω** | 2.697 |
| 1.00 mm | 0.15 mm | 1.5 mm | 4.3 | 50.97 Ω | 2.703 |

**51.2 Ω → desviación de +2.4 % respecto a 50 Ω. Correcto.** (|Γ| = 0.012 → RL −38 dB: irrelevante frente a la antena.)

Microstrip **puro** de 50 Ω sobre el mismo sustrato, por Hammerstad-Jensen: **w = 3.12 mm**.
> El Apéndice A de la guía dice «2.9–3.0 mm». Discrepancia de ~5 %, atribuible al modelo (H-J incluye corrección de dispersión). **No afecta a nada**: no vamos a usar microstrip puro. Se anota solo para que nadie tome el 2.9 como verificado.

### Sensibilidad al stackup real — esto sí importa

Los 51.2 Ω dependen de `h = 1.6 mm` y `εr = 4.3`. **JLCPCB no garantiza εr = 4.3**; su FR4 estándar de 2 capas y 1.6 mm ronda εr 4.3–4.6 según el material del lote, y el `h` real es el prepreg, no el grosor nominal de la placa. La tabla de arriba muestra que bajar `h` de 1.6 a 1.5 mm mueve Z₀ solo 0.25 Ω — **la geometría es poco sensible a `h`**, lo cual es una buena noticia.

**Obligación para F6:** volver a correr el número en la **calculadora de impedancia de JLCPCB con el stackup exacto cotizado**, no con esta fórmula. Este cálculo verifica que la geometría de SJI es coherente; no sustituye al del fabricante.

---

## 4. Frecuencia — H-11

**Sigfox RC2 transmite en 902.2 MHz**, no en 915:

- `DS R08 §3.4.1, Tabla 3-4-1-2` (pág. 7): RC2 **902.104 / 902.2 / 902.296 MHz** (min/typ/max), nota al pie: *«RC2 902.2 +/-0.096»*.
- El exhibit FCC declara el rango de hopping **902.1375–904.6625 MHz**.
- Sensibilidad de RX RC2 (`DS §3.4.2, Tabla 3-4-2-2`, pág. 8): **−124.5 dBm** medida **a 905.2 MHz**.

Diseñar a 915 MHz en vez de 902.2 es un error de **+1.42 %** en frecuencia.

**Para la traza de 50 Ω no cambia nada** — es de banda ancha y mide ~5 mm; λg/4 a 902.2 MHz es 50.6 mm. **Para la antena sí importa** (una IFA es resonante): ver `antena-cotas.md` §5. H-11 se cierra en F5/F6, no aquí.

---

## 5. Lo que este documento **no** contiene

- Coordenadas de la traza RF. No hay cotas en ninguna fuente; solo ancho y gap. Las decide **F6**.
- El keepout del plano de tierra bajo la antena. Está en `antena-cotas.md` §4.
- Longitud máxima admisible de la traza. Ninguna fuente la acota. Regla de F6, no dato de F1.
