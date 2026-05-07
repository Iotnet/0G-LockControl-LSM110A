# App

Aplicación móvil y backend para el sistema 0G LockControl.

## Subcarpetas

### Mobile/
Aplicación móvil (Android/iOS) desarrollada en Flutter o React Native:
- Recepción de notificaciones push de apertura
- Visualización de alertas con fecha, hora e ID del dispositivo
- Historial de eventos
- Vinculación de dispositivo mediante escaneo de código QR
- Configuración de notificaciones
- Futuro: comunicación BLE para activación/configuración local

### Backend/
API y servidor para recibir callbacks de Sigfox:
- Endpoint HTTP/HTTPS para callbacks del backend Sigfox
- Almacenamiento de eventos (timestamp, device ID, payload)
- Servicio de notificaciones push
- API REST para la app móvil
- Infraestructura cloud escalable
