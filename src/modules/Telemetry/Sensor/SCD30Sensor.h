#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"

class SCD30Sensor : public TelemetrySensor
{
  private:
    // Dirección I2C del sensor SCD30
    static const uint8_t SCD30_I2C_ADDRESS = 0x61;
    
    // Comandos del sensor SCD30
    static const uint16_t SCD30_CMD_START_MEASUREMENT = 0x0010;
    static const uint16_t SCD30_CMD_STOP_MEASUREMENT = 0x0104;
    static const uint16_t SCD30_CMD_READ_MEASUREMENT = 0x0300;
    static const uint16_t SCD30_CMD_DATA_READY = 0x0202;
    static const uint16_t SCD30_CMD_SET_MEASUREMENT_INTERVAL = 0x4600;
    static const uint16_t SCD30_CMD_GET_MEASUREMENT_INTERVAL = 0x4600;
    static const uint16_t SCD30_CMD_SET_TEMPERATURE_OFFSET = 0x5403;
    static const uint16_t SCD30_CMD_GET_TEMPERATURE_OFFSET = 0x5403;
    static const uint16_t SCD30_CMD_SET_ALTITUDE_COMPENSATION = 0x5102;
    static const uint16_t SCD30_CMD_GET_ALTITUDE_COMPENSATION = 0x5102;
    static const uint16_t SCD30_CMD_SET_PRESSURE_COMPENSATION = 0x5306;
    static const uint16_t SCD30_CMD_GET_PRESSURE_COMPENSATION = 0x5306;
    static const uint16_t SCD30_CMD_SET_AUTO_CALIBRATION = 0x5306;
    static const uint16_t SCD30_CMD_GET_AUTO_CALIBRATION = 0x5306;
    static const uint16_t SCD30_CMD_FORCED_RECALIBRATION = 0x5204;
    static const uint16_t SCD30_CMD_GET_FORCED_RECALIBRATION = 0x5204;
    static const uint16_t SCD30_CMD_SOFT_RESET = 0xD304;
    static const uint16_t SCD30_CMD_GET_FIRMWARE_VERSION = 0xD100;

    // Variables para almacenar las mediciones
    float co2_ppm;
    float temperature_celsius;
    float humidity_percent;
    
    // Métodos auxiliares
    bool writeCommand(uint16_t command);
    bool writeCommand(uint16_t command, uint16_t argument);
    bool readData(uint8_t *data, uint8_t length);
    uint8_t calculateCRC8(uint8_t data[], uint8_t len);
    bool dataReady();
    bool readMeasurement();

  protected:
    virtual void setup() override;

  public:
    SCD30Sensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
};

#endif

