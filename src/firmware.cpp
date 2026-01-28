#include <Arduino.h>
#include "config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <EEPROM.h>

#include "MotorController.h"
#include "SensorManager.h"
#include "Steering.h"
#include "Communication.h"
#include "WiFiPortal.h"

MotorController motorController(MOTOR_LEFT_PWM, MOTOR_LEFT_DIRECTION, MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIRECTION);
SensorManager sensorManager;
Steering steering;

void setup() {
   Serial.begin(115200);

   LOG_I("Wheel Bot Starting...\n");

   // Check portal flag in EEPROM
   uint8_t portalFlag = 0;
   EEPROM.begin(512);
   EEPROM.get(EEPROM_PORTAL_FLAG_ADDRESS, portalFlag);
   if (portalFlag == 1) {
       LOG_I("Portal flag set. Clearing flag and starting portal...\n");
       portalFlag = 0;
       EEPROM.put(EEPROM_PORTAL_FLAG_ADDRESS, portalFlag);
       EEPROM.end();

       WiFiPortal portal("Wheelbot-Ctrl-Setup");
       if (!portal.run()) {
           LOG_E("Portal failed. Restarting...\n");
           ESP.restart();
       }
       LOG_I("Portal completed. Restarting to apply settings...\n");
       delay(1000);
       ESP.restart();
   }
   EEPROM.end();

    // Load I2C addresses, shunt resistance and max current from config
    float shunt_resistance = 0.1;
    float max_current = 0.8;
    uint8_t mpu_address = 0x68;
    uint8_t ina226_address = 0x40;
    if (LittleFS.begin()) {
       File configFile = LittleFS.open("/config.json", "r");
       if (configFile) {
           DynamicJsonDocument doc(512);
            deserializeJson(doc, configFile);
            configFile.close();
            shunt_resistance = doc["shunt_resistance"] | 0.1;
            max_current = doc["max_current"] | 0.8;
            mpu_address = (uint8_t)strtol(doc["mpu_address"] | "0x68", NULL, 0);
            ina226_address = (uint8_t)strtol(doc["ina226_address"] | "0x40", NULL, 0);
        }
    }

    motorController.begin();
   LOG_I("Motor Controller Initialized.\n");

    steering.begin();
    LOG_I("Steering Initialized.\n");

    sensorManager.begin(shunt_resistance, max_current, mpu_address, ina226_address);
    LOG_I("Sensor Manager Initialized with shunt %.2f Ohm, max current %.2f A, MPU@0x%02X, INA226@0x%02X.\n", shunt_resistance, max_current, mpu_address, ina226_address);

  Communication::setup(&motorController, &sensorManager, &steering);
  LOG_I("Communication Initialized.\n");

   // Check if WiFi settings are configured
   String ssid = "";
   String password = "";
   if (LittleFS.begin()) {
        File configFile = LittleFS.open("/config.json", "r");
        if (configFile) {
            DynamicJsonDocument doc(512);
            deserializeJson(doc, configFile);
            configFile.close();
             ssid = doc["ssid"] | "";
             password = doc["password"] | "";
             shunt_resistance = doc["shunt_resistance"] | 0.1;
             max_current = doc["max_current"] | 0.8;
             mpu_address = (uint8_t)strtol(doc["mpu_address"] | "0x68", NULL, 0);
             ina226_address = (uint8_t)strtol(doc["ina226_address"] | "0x40", NULL, 0);
         }
     }

  if (ssid.length() == 0) {
    LOG_I("No WiFi settings found. Starting portal...\n");
    WiFiPortal portal("Wheelbot-Ctrl-Setup");
    if (!portal.run()) {
      LOG_E("Portal failed. Restarting...\n");
      ESP.restart();
    }
  } else {
    LOG_I("WiFi settings found (SSID: %s). Connecting...\n", ssid.c_str());
    // Wait for connection with timeout
    unsigned long connect_timeout = 60000; // 60 seconds
    unsigned long start_time = millis();
    bool connected = false;
    while (millis() - start_time < connect_timeout) {
      if (Communication::isConnected()) {
        connected = true;
        LOG_I("Connected successfully.\n");
        break;
      }
      delay(1000);
      LOG_I("Waiting for connection...\n");
    }
    if (!connected) {
      LOG_W("Connection failed. Starting portal...\n");
      WiFiPortal portal("Wheelbot-Ctrl-Setup");
      if (!portal.run()) {
        LOG_E("Portal failed. Restarting...\n");
        ESP.restart();
      }
    }
  }
  }

long last_publish = 0;

//float getRandomFloat(float min, float max) {
//  return min + (max - min) * (random(0, 2147483647) / 2147483647.0f);
// }

void publishParameters() {
  long now = millis();
  if (!Communication::isConnected() || !(now - last_publish > PUB_DELAY)) {
    return;
  }
  last_publish = now;

  JsonDocument sensors;

  JsonObject sonarsNode = sensors["sonars"].to<JsonObject>();
  sonarsNode["right"] = sensorManager.getSonarRight();
  sonarsNode["left"] = sensorManager.getSonarLeft();

  JsonObject accelerometrNode = sensors["accelerometr"].to<JsonObject>();
  accelerometrNode["x"] = sensorManager.getAccelX();
  accelerometrNode["y"] = sensorManager.getAccelY();
  accelerometrNode["z"] = sensorManager.getAccelZ();

  JsonObject gyroscopeNode = sensors["gyroscope"].to<JsonObject>();
  gyroscopeNode["x"] = sensorManager.getGyroX();
  gyroscopeNode["y"] = sensorManager.getGyroY();
  gyroscopeNode["z"] = sensorManager.getGyroZ();

  JsonObject anglesNode = sensors["angles"].to<JsonObject>();
  anglesNode["yaw"] = sensorManager.getYaw();
  anglesNode["pitch"] = sensorManager.getPitch();
  anglesNode["roll"] = sensorManager.getRoll();

  JsonObject powerNode = sensors["power"].to<JsonObject>();
  powerNode["voltage"] = sensorManager.getVoltage();
  powerNode["current"] = sensorManager.getCurrent();
  powerNode["power"] = sensorManager.getPower();
  powerNode["energy"] = sensorManager.getEnergy();

  String sensors_output;
  serializeJson(sensors, sensors_output);

  LOG_D("Published sensor parameters: %s\n", sensors_output.c_str());

  #if defined(ENABLE_SEND_DATA)
    Communication::publish("sensors/json", sensors_output);
  #endif

  JsonDocument control;

  JsonObject motorsNode = control["engines"].to<JsonObject>();
  JsonObject leftMotorNode = motorsNode["left"].to<JsonObject>();
  leftMotorNode["speed"] = motorController.getCurrentLeftSpeed();
  leftMotorNode["direction"] = motorController.getLeftDirection();
  leftMotorNode["acceleration"] = motorController.getLeftAcceleration();
  JsonObject rightMotorNode = motorsNode["right"].to<JsonObject>();
  rightMotorNode["speed"] = motorController.getCurrentRightSpeed();
  rightMotorNode["direction"] = motorController.getRightDirection();
  rightMotorNode["acceleration"] = motorController.getRightAcceleration();

  JsonObject steeringWheelNode = control["steering"].to<JsonObject>();
  steeringWheelNode["direction"] = steering.getAngle();
  steeringWheelNode["acceleration"] = steering.getAcceleration();

  String control_output;
  serializeJson(control, control_output);

  LOG_D("Published control parameters: %s\n", control_output.c_str());

  #if defined(ENABLE_SEND_DATA)
    Communication::publish("control/json", control_output);
  #endif
}

void loop() {
  sensorManager.update();
  motorController.update();
  steering.update();
  Communication::loop();

  if (Communication::restartRequested()) {
    delay(1000);
    ESP.restart();
  }

  if (Communication::portalRequested()) {
    LOG_I("Portal requested via MQTT. Setting portal flag and restarting...\n");
    uint8_t portalFlag = 1;
    EEPROM.begin(512);
    EEPROM.put(EEPROM_PORTAL_FLAG_ADDRESS, portalFlag);
    EEPROM.end();
    delay(1000);
    ESP.restart();
  }

  publishParameters();
}
