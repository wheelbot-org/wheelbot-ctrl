#include <Arduino.h>
#include "config.h"
#include "SensorManager.h"
#include <EEPROM.h>
#include <INA226.h>

SensorManager::SensorManager() :
    _mpu(),
    _sonarRight(SONAR_RIGHT_PING, SONAR_RIGHT_PING, MAX_DISTANCE),
    _sonarLeft(SONAR_LEFT_PING, SONAR_LEFT_PING, MAX_DISTANCE),
    _ina226(INA226_ADDRESS),
    _mpuAddress(MPU_ADDRESS),
    _ina226Address(INA226_ADDRESS)
{
    _last_mpu_calculate = 0;
    _last_energy_time = 0;
    _energy = 0.0;
}

void SensorManager::begin(float shunt, float maxCurrent, uint8_t mpuAddr, uint8_t inaAddr) {
    _mpuAddress = mpuAddr;
    _ina226Address = inaAddr;

    Wire.begin(SW_I2C_SDA, SW_I2C_SCL);

    EEPROM.begin(512);

    _mpu = MPU6050(_mpuAddress);
    _mpu.initialize();
    _mpu.dmpInitialize();
    _mpu.setDMPEnabled(true);

    readOffsetsMPU();

    _ina226 = INA226(_ina226Address);
    if (!_ina226.begin()) {
        LOG_E("INA226 not found at address 0x%X\n", _ina226Address);
    } else {
        _ina226.setMaxCurrentShunt(maxCurrent, shunt);
        _ina226.setModeShuntBusContinuous();
        _ina226.setBusVoltageConversionTime(INA226_1100_us);
        _ina226.setShuntVoltageConversionTime(INA226_1100_us);
        _ina226.setAverage(INA226_4_SAMPLES);

        LOG_I("INA226 initialized:\n");
        LOG_I("  Shunt: %.2f Ohm\n", shunt);
        LOG_I("  Max Current: %.2f A\n", maxCurrent);
        LOG_I("  Mode: %d (continuous)\n", _ina226.getMode());
        LOG_I("  Calibrated: %s\n", _ina226.isCalibrated() ? "yes" : "no");
        LOG_I("  Max Measurable: %.2f A\n", _ina226.getMaxCurrent());
    }
}

void SensorManager::update() {
    static unsigned long last_update = 0;
    unsigned long now = millis();
    if (now - last_update < SENSOR_UPDATE_INTERVAL) {
        return;
    }
    last_update = now;

    mpuCalculate();
    _sonarRightValue = _sonarRight.ping_cm();
    delay(30); // Wait for sonar ping
    _sonarLeftValue = _sonarLeft.ping_cm();

    // Read INA226 data
    _voltage = _ina226.getBusVoltage();
    _current = _ina226.getCurrent();
    _power = _ina226.getPower();
    if (_last_energy_time > 0) {
        unsigned long delta_time = now - _last_energy_time;
        _energy += _power * (delta_time / 3600000.0); // Wh
    }
    _last_energy_time = now;
}

void SensorManager::readOffsetsMPU() {
    int offsets[6];
    EEPROM.get(EEPROM_START_ADDRESS, offsets);
    _mpu.setXAccelOffset(offsets[0]);
    _mpu.setYAccelOffset(offsets[1]);
    _mpu.setZAccelOffset(offsets[2]);
    _mpu.setXGyroOffset(offsets[3]);
    _mpu.setYGyroOffset(offsets[4]);
    _mpu.setZGyroOffset(offsets[5]);
}

void SensorManager::calibrateMPU() {
    long offsets[6];
    long offsetsOld[6];
    int16_t mpuGet[6];

    _mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    _mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);

    _mpu.setXAccelOffset(0);
    _mpu.setYAccelOffset(0);
    _mpu.setZAccelOffset(0);
    _mpu.setXGyroOffset(0);
    _mpu.setYGyroOffset(0);
    _mpu.setZGyroOffset(0);
    delay(5);

    for (byte n = 0; n < 10; n++) {
        for (byte j = 0; j < 6; j++) {
            offsets[j] = 0;
        }

        for (byte i = 0; i < 100 + MPU_CALIBRATION_BUFFER_SIZE; i++) {
            _mpu.getMotion6(&mpuGet[0], &mpuGet[1], &mpuGet[2], &mpuGet[3], &mpuGet[4], &mpuGet[5]);
            if (i >= 99) {
                for (byte j = 0; j < 6; j++) {
                    offsets[j] += (long)mpuGet[j];
                }
            }
        }

        for (byte i = 0; i < 6; i++) {
            offsets[i] = offsetsOld[i] - ((long)offsets[i] / MPU_CALIBRATION_BUFFER_SIZE);
            if (i == 2) offsets[i] += 16384;
            offsetsOld[i] = offsets[i];
        }

        _mpu.setXAccelOffset(offsets[0] / 8);
        _mpu.setYAccelOffset(offsets[1] / 8);
        _mpu.setZAccelOffset(offsets[2] / 8);
        _mpu.setXGyroOffset(offsets[3] / 4);
        _mpu.setYGyroOffset(offsets[4] / 4);
        _mpu.setZGyroOffset(offsets[5] / 4);
        delay(2);
    }

    for (byte i = 0; i < 6; i++) {
        if (i < 3) offsets[i] /= 8;
        else offsets[i] /= 4;
    }

    EEPROM.put(EEPROM_START_ADDRESS, offsets);
}

void SensorManager::mpuCalculate() {
    long now = millis();
    if (now - _last_mpu_calculate <= MPU_MESUAREMENT_DELAY) {
        return;
    }
    _last_mpu_calculate = now;

    if (!_mpu.dmpGetCurrentFIFOPacket(_mpuFifoBuffer)) {
        return;
    }

    Quaternion q;
    VectorFloat gravity;
    VectorInt16 gyro;
    VectorInt16 accel;
    VectorInt16 accelReal;

    _mpu.dmpGetQuaternion(&q, _mpuFifoBuffer);
    _mpu.dmpGetGravity(&gravity, &q);
    _mpu.dmpGetYawPitchRoll(_mpuYPR, &q, &gravity);

    _mpu.dmpGetGyro(&gyro, _mpuFifoBuffer);
    _mpu.dmpGetAccel(&accel, _mpuFifoBuffer);
    _mpu.dmpGetLinearAccel(&accelReal, &accel, &gravity);

    _accelX = static_cast<double>(accelReal.x) / MPU_METRIC_DEVIDER * 2;
    _accelY = static_cast<double>(accelReal.y) / MPU_METRIC_DEVIDER * 2;
    _accelZ = static_cast<double>(accelReal.z) / MPU_METRIC_DEVIDER * 2;
    _gyroX = static_cast<double>(gyro.x) / MPU_METRIC_DEVIDER * 250;
    _gyroY = static_cast<double>(gyro.y) / MPU_METRIC_DEVIDER * 250;
    _gyroZ = static_cast<double>(gyro.z) / MPU_METRIC_DEVIDER * 250;
}
