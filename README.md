# 0G LockControl - Dispositivo IoT de Monitoreo de Apertura

> Diseño y desarrollo de una contrachapa IoT con tecnología Sigfox basada en el SoC LSM110A

## Descripción

**0G LockControl** es un dispositivo electrónico autónomo de monitoreo de apertura de puertas con comunicación a través de la red **Sigfox (0G)**. El sistema detecta la apertura de una puerta mediante un sensor magnético (reed switch) y transmite una alerta en tiempo real que llega al usuario final a través de una aplicación móvil.

El dispositivo está diseñado para ofrecer:
- Ultra bajo consumo energético (operación con batería por meses/años)
- Comunicación LPWAN vía Sigfox (sin dependencia de WiFi o red celular)
- Tamaño compacto e instalación sencilla
- Costo reducido de operación y manufactura

## Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        ARQUITECTURA 0G LOCKCONTROL                      │
│                                                                         │
│  ┌──────────────────────┐    ┌──────────────┐    ┌───────┐   ┌───────┐ │
│  │     DISPOSITIVO       │    │   GATEWAY     │    │ CLOUD │   │  APP  │ │
│  │                       │    │   SIGFOX      │    │       │   │ MÓVIL │ │
│  │  ┌─────────────────┐ │    │               │    │       │   │       │ │
│  │  │  Reed Switch     │ │    │    ╱╲  ╱╲    │    │  ☁️   │   │  📱  │ │
│  │  │  (Sensor Mag.)   │─┼───▶│   ╱  ╲╱  ╲   │───▶│       │──▶│       │ │
│  │  └─────────────────┘ │    │  ╱    ╱╲   ╲  │    │       │   │       │ │
│  │  ┌─────────────────┐ │    │ ╱    ╱  ╲   ╲ │    │       │   │       │ │
│  │  │  LSM110A (SoC)   │ │    └──────────────┘    └───────┘   └───────┘ │
│  │  │  STM32WL Core    │ │                                              │
│  │  │  + Radio Sigfox  │ │         Red Sigfox          Callback          │
│  │  └─────────────────┘ │         (868 MHz)            HTTP/S            │
│  │  ┌─────────────────┐ │                                                │
│  │  │  Batería         │ │                                                │
│  │  └─────────────────┘ │                                                │
│  └──────────────────────┘                                                │
└─────────────────────────────────────────────────────────────────────────┘
```

### Flujo de Operación

1. **Detección**: El reed switch detecta la apertura de la puerta (cambio de campo magnético)
2. **Interrupción**: El LSM110A sale del modo de bajo consumo (STOP/Sleep) vía EXTI
3. **Transmisión**: Se envía un mensaje Sigfox (payload de 12 bytes máx.) con ID del evento
4. **Entrega**: El backend Sigfox ejecuta un callback HTTP hacia el servidor de la app
5. **Notificación**: La app móvil muestra la alerta con fecha, hora e ID del dispositivo

## Componentes Principales

| Componente | Descripción | Referencia |
|---|---|---|
| **SoC** | LSM110A (STM32WL core, Cortex-M4 + radio Sub-GHz) | [Support-SJI/LSM110A](https://github.com/Support-SJI/LSM110A) |
| **Sensor** | Reed switch magnético (normalmente cerrado) | Sensor ABS puerta/ventana |
| **Antena** | Antena Sigfox 868 MHz (PCB trace o chip antenna) | Por definir |
| **Alimentación** | Batería (LiPo o CR2032, por evaluar) | Por definir |
| **Programador** | ST-LINK externo o UART | STM32CubeProgrammer |

## Plataforma de Desarrollo

| Herramienta | Versión | Uso |
|---|---|---|
| STM32CubeIDE | v1.18.1+ | IDE principal de desarrollo |
| STM32CubeMX | - | Configuración de periféricos (.ioc) |
| STM32CubeProgrammer | v2.19.0 | Programación y verificación de firmware |
| STM32CubeWL | v1.3.x | Paquete de firmware (HAL, BSP, stacks) |

## Estructura del Repositorio

```
0G-LockControl-LSM110A/
├── Hardware/
│   ├── Schematic/          # Esquemáticos del circuito (Altium/KiCad)
│   ├── PCB/                # Diseño de la tarjeta de circuito impreso
│   ├── BOM/                # Lista de materiales (Bill of Materials)
│   └── Datasheets/         # Hojas de datos de componentes clave
├── Firmware/
│   ├── src/                # Código fuente principal (main.c, callbacks, etc.)
│   ├── libs/               # Librerías externas y stacks (Sigfox, HAL)
│   ├── config/             # Archivos de configuración (.ioc, linker scripts)
│   └── tools/              # Scripts auxiliares (programación, pruebas)
├── 3D-Models/
│   ├── Enclosure/          # Diseño de la carcasa (STL, STEP)
│   └── Assembly/           # Ensamblaje completo del dispositivo
├── Tests/
│   ├── Lab/                # Pruebas de laboratorio (RF, consumo, funcionales)
│   └── Field/              # Pruebas de campo (instalaciones reales)
├── App/
│   ├── Mobile/             # Aplicación móvil (Flutter/React Native)
│   └── Backend/            # API y servidor para callbacks Sigfox
├── docs/
│   ├── Architecture/       # Documentos de arquitectura y decisiones de diseño
│   ├── Images/             # Diagramas, fotos, capturas
│   ├── References/         # PDFs de referencia, application notes
│   └── Guides/             # Guías de usuario, instalación, manufactura
├── .gitignore
└── README.md               # Este archivo
```

## Antecedentes del Desarrollo

Este proyecto es una evolución del trabajo realizado con la placa **NUCLEO-WL55JC2**, donde se validaron los siguientes hitos:

- Conexión exitosa a la red Sigfox (registro en backend, envío de mensajes)
- Conexión exitosa a LORIOT vía LoRaWAN (validación de stack)
- Implementación de detección de flanco con reed switch en PB1
- Envío de payload Sigfox al detectar apertura (función `EnviarMensajeFranco`)
- Caracterización del microcontrolador y periféricos del STM32WL55
- Pruebas con comandos AT para Sigfox

La transición al **LSM110A** permite pasar de una placa de desarrollo a un SoC integrado, habilitando el diseño de una PCB compacta y de bajo costo para producción.

## Protocolos de Comunicación

### Sigfox (Protocolo Principal)
- **Zona**: RCZ2 (América Latina / México)
- **Frecuencia**: 902 MHz (uplink)
- **Mensajes**: Hasta 140 mensajes/día (uplink), 4 downlink/día
- **Payload**: Máximo 12 bytes por mensaje
- **Alcance**: Hasta 50 km en zona rural, 3-10 km en zona urbana

### Comunicación Local (Futuro)
- BLE (Bluetooth Low Energy) para activación y configuración desde la app móvil

## Empresa

**0G IoT Net Solutions** - [iotnet.mx](https://iotnet.mx)

## Desarrollador

**José Francisco Díaz Figueroa** - Hardware Developer  
jdiaz@iotnet.mx

## Referencias

- [Documentación STM32WL Series](https://www.st.com/en/microcontrollers-microprocessors/stm32wl-series.html)
- [Repositorio LSM110A](https://github.com/Support-SJI/LSM110A)
- [Sigfox Build - Development](https://build.sigfox.com/development)
- [Sigfox Build - Industrialization](https://build.sigfox.com/industrialization#the-sigfox-credentials)
- [STM32CubeWL GitHub](https://github.com/STMicroelectronics/STM32CubeWL)
- [RF Board Layout Guide (AN5457)](https://www.st.com/resource/en/application_note/dm00660594.pdf)

## Licencia

Por definir
