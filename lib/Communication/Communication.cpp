#include "Communication.h"
#include "config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// TODO: Move credentials to a more secure location
String wifi_ssid, wifi_pass, mqtt_server, mqtt_port_str, device_id;
EspMQTTClient* client = nullptr;

MotorController* _motorController;
SensorManager* _sensorManager;
Steering* _steering;

void onConnectionEstablished() {
  LOG_I("MQTT connected, subscribing to topics...\n");
  client->subscribe("service/calibrate-mcu", [] (const String &payload)  {
      LOG_I("Remote calibration command accepted. Start Calibration...");
    _sensorManager->calibrateMPU();

    JsonDocument response;
    response["accel-x"] = _sensorManager->getAccelXOffset();
    response["accel-y"] = _sensorManager->getAccelYOffset();
    response["accel-z"] = _sensorManager->getAccelZOffset();
    response["gyro-x"] = _sensorManager->getGyroXOffset();
    response["gyro-y"] = _sensorManager->getGyroYOffset();
    response["gyro-z"] = _sensorManager->getGyroZOffset();

    String output;
    serializeJson(response, output);
    client->publish("service/calibrate-mcu-result", output);
  });

  client->subscribe("service/restart", [] (const String &payload)  {
      LOG_I("Remote restart command accepted. Restarting...");
    Communication::requestRestart();
  });
  
  client->subscribe("engines/left/speed_percent", [] (const String &payload)  {
    LOG_I("engines/left/speed_percent -> %d\n", payload.toInt());
    _motorController->setLeftSpeedPercent(payload.toInt());
  });

  client->subscribe("engines/right/speed_percent", [] (const String &payload)  {
    LOG_I("engines/right/speed_percent -> %d\n", payload.toInt());
    _motorController->setRightSpeedPercent(payload.toInt());
  });

  client->subscribe("engines/left/acceleration", [] (const String &payload)  {
    LOG_I("engines/left/acceleration -> %d\n", payload.toInt());
    _motorController->setLeftAcceleration(payload.toInt());
  });

  client->subscribe("engines/right/acceleration", [] (const String &payload)  {
    LOG_I("engines/right/acceleration -> %d\n", payload.toInt());
    _motorController->setRightAcceleration(payload.toInt());
  });

  client->subscribe("steering-wheel/rotate", [] (const String &payload)  {
    LOG_I("steering-wheel/rotate -> %d\n", payload.toInt());
    _steering->setAngle(payload.toInt());
  });

  client->subscribe("steering-wheel/acceleration", [] (const String &payload)  {
    LOG_I("steering-wheel/acceleration -> %d\n", payload.toInt());
    _steering->setAcceleration(payload.toInt());
  });
}

namespace Communication {

bool _restart_requested = false;

void setup(MotorController* motorController, SensorManager* sensorManager, Steering* steering) {
    _motorController = motorController;
    _sensorManager = sensorManager;
    _steering = steering;

    LOG_I("Reading config from NVS...\n");
    if (!LittleFS.begin()) {
        LOG_E("Failed to mount LittleFS\n");
        wifi_ssid = "ssid-default";
        wifi_pass = "pass-default";
        mqtt_server = "dev.rightech.io";
        mqtt_port_str = "1883";
        device_id = "wheelbot-default";
    } else {
        File configFile = LittleFS.open("/config.json", "r");
        if (configFile) {
            DynamicJsonDocument doc(512);
            deserializeJson(doc, configFile);
            configFile.close();
            wifi_ssid = doc["ssid"] | "ssid-default";
            wifi_pass = doc["password"] | "pass-default";
            mqtt_server = doc["server"] | "dev.rightech.io";
            mqtt_port_str = doc["server_port"] | "1883";
            device_id = doc["device_id"] | "wheelbot-default";
            LOG_I("Config loaded: SSID=%s, Server=%s:%s, ID=%s\n", wifi_ssid.c_str(), mqtt_server.c_str(), mqtt_port_str.c_str(), device_id.c_str());
        } else {
            LOG_W("Config file not found, using defaults\n");
            wifi_ssid = "ssid-default";
            wifi_pass = "pass-default";
            mqtt_server = "dev.rightech.io";
            mqtt_port_str = "1883";
            device_id = "wheelbot-default";
        }
    }

    LOG_I("Creating MQTT client...\n");
    client = new EspMQTTClient(wifi_ssid.c_str(), wifi_pass.c_str(), mqtt_server.c_str(), device_id.c_str(), atoi(mqtt_port_str.c_str()));

    client->enableMQTTPersistence();
    client->setOnConnectionEstablishedCallback(onConnectionEstablished);

}

void loop() {
    if (client) client->loop();
}

void publish(const char* topic, const String& payload) {
    if (client) client->publish(topic, payload);
}

bool isConnected() {
    client->loop();
    return client && client->isConnected();
}

bool restartRequested() {
    if (_restart_requested) {
        _restart_requested = false; // Reset the flag
        return true;
    }
    return false;
}

void requestRestart() {
    _restart_requested = true;
}

} // namespace Communication
