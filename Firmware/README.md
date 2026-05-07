# Firmware

Código fuente y configuración del firmware para el LSM110A (STM32WL core).

## Subcarpetas

### src/
Código fuente principal: `main.c`, handlers de interrupción, rutinas de transmisión Sigfox, gestión de bajo consumo, y detección de flancos del reed switch.

### libs/
Librerías y stacks externos:
- HAL STM32WLxx (Hardware Abstraction Layer)
- Stack Sigfox (SIGFOX_API)
- BSP del LSM110A
- CMSIS

### config/
Archivos de configuración del proyecto:
- `.ioc` (STM32CubeMX pinout y periféricos)
- Linker scripts (`.ld`)
- Archivos de configuración de credenciales Sigfox (ID, PAC, KEY)

### tools/
Scripts auxiliares:
- Scripts de programación (flash via UART/SWD)
- Scripts de pruebas automatizadas
- Utilidades de compilación

## Flujo del Firmware

```
[RESET/Power On]
       │
       ▼
[Inicialización]
  - HAL_Init()
  - SystemClock_Config()
  - GPIO_Init() (reed switch en EXTI)
  - Sigfox_Init()
       │
       ▼
[Modo STOP/Sleep] ◄──────────────┐
  (ultra bajo consumo)            │
       │                          │
  [Interrupción EXTI]            │
  (flanco del reed switch)       │
       │                          │
       ▼                          │
[Procesar Evento]                │
  - Debounce (filtro anti-rebote) │
  - Armar payload Sigfox          │
  - SIGFOX_API_send_frame()       │
  - Confirmar transmisión         │
       │                          │
       └──────────────────────────┘
```

## Referencia del Código Base (NUCLEO-WL55JC2)

El firmware se basa en el código validado con la placa NUCLEO:
- Detección de flanco en PB1 (reed switch)
- Función `GestionarFlancoPB1()` con anti-rebote
- Función `EnviarMensajeFranco()` para transmisión Sigfox
- Configuración de reloj MSI a 32 MHz

## Herramientas

| Herramienta | Uso |
|---|---|
| STM32CubeIDE v1.18.1+ | Compilación y depuración |
| STM32CubeProgrammer | Flash del binario |
| Repositorio LSM110A | [Support-SJI/LSM110A](https://github.com/Support-SJI/LSM110A) |
