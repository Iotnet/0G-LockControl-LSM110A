# Especificación de producto — Alarma IoT dual (acelerómetro + magnético)

**Proyecto:** LSM110A Alarm Device  
**Versión:** 1.0 (decisiones resueltas)  
**Fecha:** 2026-05-07  
**Estado:** Aprobado para MVP

---

## 1. Descripción general

Dispositivo compacto de alarma inalámbrica que detecta movimiento/vibración (acelerómetro I2C) y apertura de puerta/ventana (sensor magnético). Transmite alertas por Sigfox RC2 (902-928 MHz, México). Event-driven: duerme en ultra-bajo consumo y solo despierta por interrupción de hardware.

**Caso de uso:** Se adhiere a puerta, ventana, gabinete o equipo en interior (oficina, bodega, cuarto frío). Al detectar evento, envía alerta a la nube Sigfox → notificación al usuario.

## 2. Decisiones técnicas resueltas

| # | Pregunta | Decisión | Justificación |
|---|----------|----------|---------------|
| P1 | Reed vs Hall | Ambos footprints en PCB | Flexibilidad; mismo pin EXTI |
| P2 | Batería | CR2450 + cap soporte 470µF | Tamaño compacto; cap compensa pulso TX |
| P3 | LDO | Footprint con bypass 0Ω | Probar directo vs regulado sin re-hacer PCB |
| P4 | Umbral accel | 200mg default, configurable | Balance sensibilidad vs falsos positivos |
| P4b | Cooldown | 60 seg (max 1 msg/min) | Protege límite diario Sigfox |
| P5 | Downlink | Estructura preparada, no implementar | Struct en flash listo; parser para v2 |
| P6 | NOM/IFT | Post-MVP | NOM-208/IFT-008 aplica; LSM110A tiene FCC |
| P7 | Heartbeat | Cada 24 horas | Máxima vida de batería |
| P8 | Potencia TX | Configurable, default +14dBm | ~50mA, seguro con CR2450 |

## 3. Arquitectura del sistema

### Ciclo de operación

```
Sleep (3µA) → Wake por INT → Leer sensor → Armar payload → TX Sigfox (~50mA x 2s) → Sleep
```

### Bloques del sistema

| Bloque | Componente | Conexión | Función |
|--------|-----------|----------|---------|
| Procesador + Radio | LSM110A (STM32WL55) | Integrado | MCU Cortex-M4 + radio sub-GHz |
| Acelerómetro | LIS2DW12 | I2C: PA9/PA10 | Detección movimiento |
| Wake-up accel | LIS2DW12 INT1 | PA0 (EXTI0) | Interrupción hardware |
| Sensor magnético A | Reed switch NA | PA1 (EXTI1) | Opción A: apertura puerta |
| Sensor magnético B | DRV5032 hall | PA1 (EXTI1) | Opción B: apertura puerta |
| Alimentación | CR2450 3V 620mAh | Pin 11 VDD | Batería desechable |
| Cap soporte | 470µF | Paralelo VDD | Pulso TX |
| Regulador (opc.) | TPS7A02 + bypass 0Ω | Entre bat y VDD | Regulación opcional |
| Antena | Traza PCB 50Ω | Pin 33 RFOUT | 902-928 MHz |
| Debug | Header SWD 4 pines | PA13/PA14 | Programación |

### Asignación de pines

| Pin | GPIO | Función | Config |
|-----|------|---------|--------|
| 11 | VDD | Alimentación | 3.0V directo o 2.5V LDO |
| 1,10,12,20,23,32,34 | GND | Tierra | Plano GND |
| 7 | PA13 | SWDIO | Debug SWD |
| 8 | PA14 | SWCLK | Debug SWD |
| 26 | PA9 | I2C1_SCL | Pullup 4.7kΩ |
| 27 | PA10 | I2C1_SDA | Pullup 4.7kΩ |
| 14 | PA0 | EXTI0 | INT1 acelerómetro |
| 15 | PA1 | EXTI1 | Sensor magnético |
| 18 | PB7 | UART1_RX | Debug (no poblar prod) |
| 19 | PB6 | UART1_TX | Debug (no poblar prod) |
| 16 | PA2 | GPIO | LED debug |
| 30 | NRST | Reset | Cap 100nF a GND |
| 31 | BOOT0 | Boot | Flotante |
| 33 | RF_OUT | RF 50Ω | Traza antena |

## 4. Payload Sigfox (12 bytes)

| Byte | Campo | Tipo | Descripción |
|------|-------|------|-------------|
| 0 | Tipo mensaje | uint8 | 0x01=alarma, 0x02=heartbeat |
| 1 | Fuente evento | bitfield | bit0=accel, bit1=magnético |
| 2-3 | Magnitud | uint16 BE | mg |
| 4 | Eje dominante | uint8 | 0=X, 1=Y, 2=Z |
| 5 | Estado magnético | uint8 | 0=cerrado, 1=abierto |
| 6 | Batería | uint8 | 0-100% |
| 7 | Temperatura | int8 | °C + offset 40 |
| 8-9 | Conteo eventos | uint16 BE | Desde último heartbeat |
| 10-11 | Reservado | uint16 | 0x0000 |

## 5. Config struct (preparado para downlink v2)

```c
typedef struct {
    uint16_t accel_threshold_mg;  // default 200
    uint16_t cooldown_seconds;    // default 60
    int8_t   tx_power_dbm;        // default 14
    uint8_t  heartbeat_hours;     // default 24
    uint8_t  daily_msg_limit;     // default 130
} device_config_t;
```

## 6. Plan MVP

| Milestone | Entregable | Semana | Responsable |
|-----------|-----------|--------|-------------|
| M0 | Repo + spec + CubeIDE setup | 1 | Ambos |
| M1 | Esquemático KiCad | 2-3 | Tú |
| M2 | FW en Nucleo (accel+reed+Sigfox) | 2-3 | Tu persona |
| M3 | PCB layout + gerbers | 3-4 | Tú |
| M4 | Low power validado en Nucleo | 3-4 | Tu persona |
| M5 | PCB fabricada y ensamblada | 5-6 | Fábrica |
| M6 | Integración FW en PCB custom | 6-7 | Ambos |
| M7 | Prueba de campo (1 semana) | 8 | Ambos |

## 7. Siguientes pasos (esta semana)

1. Crear repo GitHub con estructura: `docs/`, `hardware/`, `firmware/`
2. Subir esta spec como `docs/spec-producto.md`
3. Tu persona: clonar repo SJI/LSM110A, compilar SDK, flashear Nucleo
4. Tú: crear símbolo + footprint LSM110A en KiCad
5. Ambos: validar TX Sigfox desde Nucleo (confirmar cobertura)
6. Tú: conseguir CR2450 + cap 470µF, medir voltaje durante TX
