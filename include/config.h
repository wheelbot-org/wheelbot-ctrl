#ifndef CONFIG_H
#define CONFIG_H

// ==========================================================================
// ==                             DEBUG FLAGS                              ==
// ==========================================================================
#define ENABLE_LOG
//#define ENABLE_DEBUG
#define ENABLE_SEND_DATA

// ==========================================================================
// ==                             LOG LEVELS                              ==
// ==========================================================================
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_E(...) if (LOG_LEVEL >= LOG_LEVEL_ERROR) Serial.printf("[ERROR] " __VA_ARGS__)
#define LOG_W(...) if (LOG_LEVEL >= LOG_LEVEL_WARN) Serial.printf("[WARN] " __VA_ARGS__)
#define LOG_I(...) if (LOG_LEVEL >= LOG_LEVEL_INFO) Serial.printf("[INFO] " __VA_ARGS__)
#define LOG_D(...) if (LOG_LEVEL >= LOG_LEVEL_DEBUG) Serial.printf("[DEBUG] " __VA_ARGS__)


// ==========================================================================
// ==                         GENERAL SETTINGS                             ==
// ==========================================================================
#define PUB_DELAY (1 * 1000)  // 1 second


// ==========================================================================
// ==                            PIN DEFINITIONS                           ==
// ==========================================================================

// -- Motor Pins --
#define MOTOR_LEFT_PWM 4
#define MOTOR_LEFT_DIRECTION 0
#define MOTOR_RIGHT_PWM 5
#define MOTOR_RIGHT_DIRECTION 2

// -- Servo Pin --
#define STEERING_WHEEL 14

// -- Sonar Pins (for single-pin operation) --
#define SONAR_RIGHT_PING 16
#define SONAR_LEFT_PING 15

// -- I2C Pins for MPU6050 --
#define SW_I2C_SCL 12
#define SW_I2C_SDA 13


// ==========================================================================
// ==                          COMPONENT SETTINGS                          ==
// ==========================================================================

// -- Sonar Settings --
#define MAX_DISTANCE 200  // Maximum distance to ping for (in cm)

// -- MPU6050 Settings --
// Note: MPU6050 I2C address is now configurable via captive portal. Default: 0x68
#define MPU_ADDRESS 0x68
#define MPU_CALIBRATION_BUFFER_SIZE 100
#define MPU_METRIC_DEVIDER 32768
#define MPU_MESUAREMENT_DELAY 100  // Delay between MPU measurements in ms (changed to common 100ms)

// -- INA226 Settings --
// Note: INA226 I2C address is now configurable via captive portal. Default: 0x40
#define INA226_ADDRESS 0x40
#define INA226_SHUNT_RESISTANCE 0.1  // Shunt resistance in ohms (example: 0.1 ohm for current up to 3.2A)

// -- EEPROM Settings --
#define EEPROM_START_ADDRESS 0x00
#define EEPROM_PORTAL_FLAG_ADDRESS 100

// -- Motor Controller Settings --
#define MOTOR_UPDATE_INTERVAL 100 // Update motor speed every 100ms (changed to common)

// -- Steering Settings --
#define STEERING_UPDATE_INTERVAL 100 // Update steering angle every 100ms (changed to common)

// -- Sensor Manager Settings --
#define SENSOR_UPDATE_INTERVAL 100 // Common update interval for all sensors in ms

#endif // CONFIG_H
