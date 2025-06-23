# Meshtastic CO2 Firmware

Firmware de Meshtastic modificado con soporte completo para sensores de CO2, específicamente el sensor SCD30. Este proyecto está basado en el repositorio `smartcitizen-meshtastic` y añade capacidades de monitoreo de CO2 a la red mesh de Meshtastic.

## 🌟 Características

- **Soporte para sensor SCD30**: Medición de CO2, temperatura y humedad
- **Integración completa**: Compatible con la arquitectura de telemetría existente de Meshtastic
- **Comunicación I2C**: Protocolo robusto con validación CRC
- **Transmisión mesh**: Los datos de CO2 se transmiten a través de la red Meshtastic
- **Configuración automática**: Detección automática del sensor en el bus I2C

## 📋 Requisitos

### Hardware
- Dispositivo compatible con Meshtastic (ESP32, nRF52, etc.)
- Sensor SCD30 de Sensirion
- Conexión I2C entre el dispositivo y el sensor

### Software
- PlatformIO Core
- Librerías de Meshtastic (incluidas en el proyecto)

## 🔧 Instalación

### 1. Clonar el repositorio
```bash
git clone https://github.com/gabotrixinc/meshtastic-co2-firmware.git
cd meshtastic-co2-firmware
```

### 2. Instalar dependencias
```bash
# Instalar PlatformIO si no lo tienes
pip install platformio

# Inicializar submódulos
git submodule update --init
```

### 3. Compilar el firmware
```bash
# Para T-Beam
pio run -e tbeam

# Para otros dispositivos, ver platformio.ini para más opciones
pio run -e heltec-v3
pio run -e tlora-v2
```

### 4. Flashear el dispositivo
```bash
# Conectar el dispositivo y ejecutar
pio run -e tbeam --target upload
```

## 🔌 Conexión del Hardware

### Sensor SCD30
| SCD30 Pin | ESP32 Pin | Descripción |
|-----------|-----------|-------------|
| VCC       | 3.3V      | Alimentación |
| GND       | GND       | Tierra |
| SDA       | GPIO21    | Datos I2C |
| SCL       | GPIO22    | Reloj I2C |

**Dirección I2C**: 0x61 (fija)

## 📊 Datos Transmitidos

El sensor SCD30 proporciona:
- **CO2**: Concentración en ppm (partes por millón)
- **Temperatura**: En grados Celsius
- **Humedad relativa**: En porcentaje

Los datos se integran en los mensajes de telemetría de Meshtastic usando:
- `air_quality_metrics.co2` para la concentración de CO2
- `environment_metrics.temperature` para temperatura
- `environment_metrics.relative_humidity` para humedad

## 🛠️ Configuración

### Habilitación del módulo
El módulo de CO2 se habilita automáticamente cuando se detecta un sensor SCD30 en el bus I2C. También se puede configurar manualmente:

```cpp
// En la configuración del dispositivo
moduleConfig.telemetry.air_quality_enabled = true;
```

### Intervalo de medición
Por defecto, el sensor toma mediciones cada 2 segundos y transmite datos según la configuración de telemetría de Meshtastic.

## 📁 Estructura del Código

### Archivos principales añadidos/modificados:

```
src/
├── modules/Telemetry/
│   ├── CO2Telemetry.h              # Módulo de telemetría de CO2
│   ├── CO2Telemetry.cpp
│   └── Sensor/
│       ├── SCD30Sensor.h           # Driver del sensor SCD30
│       └── SCD30Sensor.cpp
├── detect/
│   ├── ScanI2C.h                   # Detección I2C (modificado)
│   └── ScanI2CTwoWire.cpp          # Implementación escáner (modificado)
├── configuration.h                 # Configuración (modificado)
├── main.cpp                        # Mapeo de sensores (modificado)
└── modules/Modules.cpp             # Registro de módulos (modificado)
```

## 🔍 Detalles Técnicos

### Clase SCD30Sensor
- Implementa el protocolo I2C del SCD30
- Maneja comandos de inicio/parada de medición
- Valida datos usando CRC-8
- Gestiona modos de bajo consumo

### Módulo CO2Telemetry
- Extiende el sistema de telemetría de Meshtastic
- Maneja la transmisión de datos de CO2
- Compatible con la arquitectura de módulos existente

### Integración I2C
- Detección automática en dirección 0x61
- Mapeo al tipo de sensor `SCD4X` en el sistema de telemetría
- Inicialización automática durante el arranque

## 🚀 Uso

1. **Flashear el firmware** en tu dispositivo Meshtastic
2. **Conectar el sensor SCD30** según el esquema de conexiones
3. **Reiniciar el dispositivo** - el sensor se detectará automáticamente
4. **Verificar en los logs** que el sensor se ha inicializado correctamente
5. **Los datos de CO2** aparecerán en la aplicación Meshtastic en la sección de telemetría

## 📱 Visualización de Datos

Los datos del sensor aparecen en:
- **Aplicación móvil Meshtastic**: Sección de telemetría
- **Interfaz web**: Panel de sensores ambientales
- **API Python**: Mediante los protocolos de telemetría

## 🤝 Contribuciones

Este proyecto está basado en:
- [Meshtastic Firmware](https://github.com/meshtastic/firmware) - Firmware base
- [SmartCitizen Meshtastic](https://github.com/fablabbcn/smartcitizen-meshtastic) - Fork base utilizado

## 📄 Licencia

Este proyecto mantiene la licencia GPL-3.0 del proyecto Meshtastic original.

## 🐛 Problemas Conocidos

- El sensor requiere un tiempo de calentamiento de ~30 segundos para lecturas estables
- La precisión puede verse afectada por cambios rápidos de temperatura
- Se recomienda calibración periódica para mediciones críticas

## 📞 Soporte

Para problemas específicos del soporte de CO2, crear un issue en este repositorio.
Para problemas generales de Meshtastic, consultar el [repositorio oficial](https://github.com/meshtastic/firmware).

---

**Desarrollado por**: Gabotrix  
**Basado en**: Meshtastic Firmware y SmartCitizen Meshtastic  
**Sensor soportado**: Sensirion SCD30

