#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "SCD30Sensor.h"
#include "TelemetrySensor.h"
#include <Wire.h>

SCD30Sensor::SCD30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_SCD4X, "SCD30")
{
    co2_ppm = 0.0;
    temperature_celsius = 0.0;
    humidity_percent = 0.0;
}

int32_t SCD30Sensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }

    // Verificar si el sensor está conectado
    Wire.beginTransmission(SCD30_I2C_ADDRESS);
    status = (Wire.endTransmission() == 0) ? 1 : 0;

    if (!status) {
        LOG_WARN("SCD30 sensor not found at address 0x%02X", SCD30_I2C_ADDRESS);
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }

    return initI2CSensor();
}

void SCD30Sensor::setup()
{
    // Detener cualquier medición en curso
    writeCommand(SCD30_CMD_STOP_MEASUREMENT);
    delay(500);

    // Configurar intervalo de medición a 2 segundos
    writeCommand(SCD30_CMD_SET_MEASUREMENT_INTERVAL, 2);
    delay(100);

    // Iniciar medición continua con compensación de presión atmosférica (1013 mbar)
    writeCommand(SCD30_CMD_START_MEASUREMENT, 1013);
    delay(100);

    LOG_INFO("SCD30 sensor initialized successfully");
}

bool SCD30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    if (!dataReady()) {
        LOG_DEBUG("SCD30 data not ready");
        return false;
    }

    if (!readMeasurement()) {
        LOG_WARN("Failed to read SCD30 measurement");
        return false;
    }

    // Configurar las métricas de calidad del aire
    measurement->variant.air_quality_metrics.has_co2 = true;
    measurement->variant.air_quality_metrics.co2 = (uint32_t)co2_ppm;

    // También podemos incluir temperatura y humedad en las métricas ambientales
    measurement->variant.environment_metrics.has_temperature = true;
    measurement->variant.environment_metrics.has_relative_humidity = true;
    measurement->variant.environment_metrics.temperature = temperature_celsius;
    measurement->variant.environment_metrics.relative_humidity = humidity_percent;

    LOG_DEBUG("SCD30 - CO2: %.1f ppm, Temp: %.1f°C, Humidity: %.1f%%", 
              co2_ppm, temperature_celsius, humidity_percent);

    return true;
}

bool SCD30Sensor::writeCommand(uint16_t command)
{
    Wire.beginTransmission(SCD30_I2C_ADDRESS);
    Wire.write(command >> 8);   // MSB
    Wire.write(command & 0xFF); // LSB
    return (Wire.endTransmission() == 0);
}

bool SCD30Sensor::writeCommand(uint16_t command, uint16_t argument)
{
    uint8_t data[2] = {(uint8_t)(argument >> 8), (uint8_t)(argument & 0xFF)};
    uint8_t crc = calculateCRC8(data, 2);

    Wire.beginTransmission(SCD30_I2C_ADDRESS);
    Wire.write(command >> 8);   // Command MSB
    Wire.write(command & 0xFF); // Command LSB
    Wire.write(data[0]);        // Argument MSB
    Wire.write(data[1]);        // Argument LSB
    Wire.write(crc);            // CRC
    return (Wire.endTransmission() == 0);
}

bool SCD30Sensor::readData(uint8_t *data, uint8_t length)
{
    Wire.requestFrom(SCD30_I2C_ADDRESS, length);
    
    if (Wire.available() != length) {
        return false;
    }

    for (uint8_t i = 0; i < length; i++) {
        data[i] = Wire.read();
    }

    return true;
}

uint8_t SCD30Sensor::calculateCRC8(uint8_t data[], uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

bool SCD30Sensor::dataReady()
{
    if (!writeCommand(SCD30_CMD_DATA_READY)) {
        return false;
    }

    delay(3); // Esperar un poco antes de leer

    uint8_t data[3];
    if (!readData(data, 3)) {
        return false;
    }

    // Verificar CRC
    uint8_t expectedCRC = calculateCRC8(data, 2);
    if (data[2] != expectedCRC) {
        LOG_WARN("SCD30 CRC mismatch in data ready check");
        return false;
    }

    uint16_t ready = (data[0] << 8) | data[1];
    return (ready == 1);
}

bool SCD30Sensor::readMeasurement()
{
    if (!writeCommand(SCD30_CMD_READ_MEASUREMENT)) {
        return false;
    }

    delay(3); // Esperar un poco antes de leer

    uint8_t data[18]; // 6 valores de 16 bits + 6 CRCs
    if (!readData(data, 18)) {
        return false;
    }

    // Verificar CRCs y extraer datos
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t offset = i * 3;
        uint8_t expectedCRC = calculateCRC8(&data[offset], 2);
        if (data[offset + 2] != expectedCRC) {
            LOG_WARN("SCD30 CRC mismatch in measurement data at position %d", i);
            return false;
        }
    }

    // Extraer valores de CO2, temperatura y humedad
    uint32_t co2_raw = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
                       ((uint32_t)data[3] << 8) | data[4];
    uint32_t temp_raw = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) | 
                        ((uint32_t)data[9] << 8) | data[10];
    uint32_t hum_raw = ((uint32_t)data[12] << 24) | ((uint32_t)data[13] << 16) | 
                       ((uint32_t)data[15] << 8) | data[16];

    // Convertir a valores float
    co2_ppm = *(float*)&co2_raw;
    temperature_celsius = *(float*)&temp_raw;
    humidity_percent = *(float*)&hum_raw;

    return true;
}

#endif

