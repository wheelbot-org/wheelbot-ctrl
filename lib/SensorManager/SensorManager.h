#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <NewPing.h>
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <INA226.h>

class SensorManager {
public:
    SensorManager();
    void begin(float shunt = 0.1, uint8_t mpuAddr = 0x68, uint8_t inaAddr = 0x40);
    void update();
    void calibrateMPU();

    // Getters
    float getYaw() { return degrees(_mpuYPR[0]); }
    float getPitch() { return degrees(_mpuYPR[1]); }
    float getRoll() { return degrees(_mpuYPR[2]); }
    double getAccelX() { return _accelX; }
    double getAccelY() { return _accelY; }
    double getAccelZ() { return _accelZ; }
    double getGyroX() { return _gyroX; }
    double getGyroY() { return _gyroY; }
    double getGyroZ() { return _gyroZ; }
    unsigned int getSonarLeft() { return _sonarLeftValue; }
    unsigned int getSonarRight() { return _sonarRightValue; }
    float getVoltage() { return _voltage; }
    float getCurrent() { return _current; }
    float getPower() { return _power; }
    float getEnergy() { return _energy; }

    // MPU Offsets for calibration result
    int16_t getAccelXOffset() { return _mpu.getXAccelOffset(); }
    int16_t getAccelYOffset() { return _mpu.getYAccelOffset(); }
    int16_t getAccelZOffset() { return _mpu.getZAccelOffset(); }
    int16_t getGyroXOffset() { return _mpu.getXGyroOffset(); }
    int16_t getGyroYOffset() { return _mpu.getYGyroOffset(); }
    int16_t getGyroZOffset() { return _mpu.getZGyroOffset(); }

private:
    void readOffsetsMPU();
    void mpuCalculate();

    MPU6050 _mpu;
    NewPing _sonarRight;
    NewPing _sonarLeft;
    INA226 _ina226;
    uint8_t _mpuAddress;
    uint8_t _ina226Address;

    uint8_t _mpuFifoBuffer[45];
    float _mpuYPR[3];
    double _accelX, _accelY, _accelZ, _gyroX, _gyroY, _gyroZ;
    unsigned int _sonarLeftValue, _sonarRightValue;
    float _voltage, _current, _power, _energy;
    unsigned long _last_energy_time;

    long _last_mpu_calculate;
};

#endif // SENSOR_MANAGER_H
