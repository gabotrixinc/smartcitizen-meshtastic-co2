#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "CO2Telemetry.h"
#include "Default.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "RTC.h"
#include "Router.h"
#include "main.h"
#include "Sensor/SCD30Sensor.h"
#include <Throttle.h>

// Instancia del sensor SCD30
SCD30Sensor scd30Sensor;

int32_t CO2TelemetryModule::runOnce()
{
    /*
        Uncomment the preferences below if you want to use the module
        without having to configure it from the PythonAPI or WebUI.
    */

    // moduleConfig.telemetry.air_quality_enabled = 1;

    if (!(moduleConfig.telemetry.air_quality_enabled)) {
        // If this module is not enabled, don't waste any OSThread time on it
        return disable();
    }

    if (firstTime) {
        // This is the first time the OSThread library has called this function, so do some setup
        firstTime = false;

        if (moduleConfig.telemetry.air_quality_enabled) {
            LOG_INFO("CO2 Telemetry: init");

            // Inicializar el sensor SCD30
            if (scd30Sensor.hasSensor()) {
                int32_t result = scd30Sensor.runOnce();
                if (result != UINT32_MAX) {
                    return setStartDelay();
                }
            }
            
            LOG_WARN("No CO2 sensor found, disabling module");
            return disable();
        }
        return disable();
    } else {
        // if we somehow got to a second run of this module with measurement disabled, then just wait forever
        if (!moduleConfig.telemetry.air_quality_enabled)
            return disable();

        if (((lastSentToMesh == 0) ||
             !Throttle::isWithinTimespanMs(lastSentToMesh, Default::getConfiguredOrDefaultMsScaled(
                                                               moduleConfig.telemetry.air_quality_interval,
                                                               default_telemetry_broadcast_interval_secs, numOnlineNodes))) &&
            airTime->isTxAllowedChannelUtil(config.device.role != meshtastic_Config_DeviceConfig_Role_SENSOR) &&
            airTime->isTxAllowedAirUtil()) {
            sendTelemetry();
            lastSentToMesh = millis();
        } else if (service->isToPhoneQueueEmpty()) {
            // Just send to phone when it's not our time to send to mesh yet
            // Only send while queue is empty (phone assumed connected)
            sendTelemetry(NODENUM_BROADCAST, true);
        }

        return sendToPhoneIntervalMs;
    }
}

bool CO2TelemetryModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_Telemetry *t)
{
    if (t->which_variant == meshtastic_Telemetry_air_quality_metrics_tag) {
#ifdef DEBUG_PORT
        const char *sender = getSenderShortName(mp);

        LOG_INFO("(Received CO2 from %s): co2=%i ppm", sender, t->variant.air_quality_metrics.co2);
#endif
        // release previous packet before occupying a new spot
        if (lastMeasurementPacket != nullptr)
            packetPool.release(lastMeasurementPacket);

        lastMeasurementPacket = packetPool.allocCopy(mp);
    }

    return false; // Let others look at this message also if they want
}

bool CO2TelemetryModule::getCO2Telemetry(meshtastic_Telemetry *m)
{
    if (!scd30Sensor.hasSensor() || !scd30Sensor.isRunning()) {
        LOG_WARN("Skip send measurements. SCD30 sensor not available");
        return false;
    }

    m->time = getTime();
    m->which_variant = meshtastic_Telemetry_air_quality_metrics_tag;
    
    // Obtener las métricas del sensor
    if (!scd30Sensor.getMetrics(m)) {
        LOG_WARN("Failed to get CO2 metrics from SCD30 sensor");
        return false;
    }

    LOG_INFO("Send: CO2=%i ppm, Temp=%.1f°C, Humidity=%.1f%%", 
             m->variant.air_quality_metrics.co2,
             m->variant.environment_metrics.temperature,
             m->variant.environment_metrics.relative_humidity);

    return true;
}

meshtastic_MeshPacket *CO2TelemetryModule::allocReply()
{
    if (currentRequest) {
        auto req = *currentRequest;
        const auto &p = req.decoded;
        meshtastic_Telemetry scratch;
        meshtastic_Telemetry *decoded = NULL;
        memset(&scratch, 0, sizeof(scratch));
        if (pb_decode_from_bytes(p.payload.bytes, p.payload.size, &meshtastic_Telemetry_msg, &scratch)) {
            decoded = &scratch;
        } else {
            LOG_ERROR("Error decoding CO2Telemetry module!");
            return NULL;
        }
        // Check for a request for air quality metrics (CO2)
        if (decoded->which_variant == meshtastic_Telemetry_air_quality_metrics_tag) {
            meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
            if (getCO2Telemetry(&m)) {
                LOG_INFO("CO2 telemetry reply to request");
                return allocDataProtobuf(m);
            } else {
                return NULL;
            }
        }
    }
    return NULL;
}

bool CO2TelemetryModule::sendTelemetry(NodeNum dest, bool phoneOnly)
{
    meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
    if (getCO2Telemetry(&m)) {
        meshtastic_MeshPacket *p = allocDataProtobuf(m);
        p->to = dest;
        p->decoded.want_response = false;
        if (config.device.role == meshtastic_Config_DeviceConfig_Role_SENSOR)
            p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
        else
            p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;

        // release previous packet before occupying a new spot
        if (lastMeasurementPacket != nullptr)
            packetPool.release(lastMeasurementPacket);

        lastMeasurementPacket = packetPool.allocCopy(*p);
        if (phoneOnly) {
            LOG_INFO("Send CO2 packet to phone");
            service->sendToPhone(p);
        } else {
            LOG_INFO("Send CO2 packet to mesh");
            service->sendToMesh(p, RX_SRC_LOCAL, true);
        }
        return true;
    }

    return false;
}

#endif

