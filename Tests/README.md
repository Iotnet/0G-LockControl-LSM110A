# Tests

Pruebas funcionales, de validación e integración del dispositivo 0G LockControl.

## Subcarpetas

### Lab/
Pruebas de laboratorio:
- Simulaciones de apertura de puerta (reed switch)
- Verificación de transmisión de mensajes Sigfox
- Análisis de señal RF (potencia, frecuencia, espectro)
- Medición de consumo energético (modo activo vs. sleep/stop)
- Validación de anti-rebote (debounce)
- Pruebas de rango con atenuador

### Field/
Pruebas de campo en instalaciones reales:
- Latencia extremo a extremo (apertura → notificación en app)
- Confiabilidad de transmisión en distintas condiciones
- Autonomía energética (duración de batería)
- Comportamiento en ambientes con interferencia RF
- Validación del montaje físico (carcasa + imán)
