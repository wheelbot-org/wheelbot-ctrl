#include <Arduino.h>
#include "config.h"
#include "MotorController.h"

MotorController::MotorController(int left_pwm_pin, int left_dir_pin, int right_pwm_pin, int right_dir_pin) {
    _left_pwm_pin = left_pwm_pin;
    _left_dir_pin = left_dir_pin;
    _right_pwm_pin = right_pwm_pin;
    _right_dir_pin = right_dir_pin;

    _current_left_speed = 0;
    _current_right_speed = 0;
    _target_left_speed = 0;
    _target_right_speed = 0;
    _left_acceleration = 5; // Default acceleration
    _right_acceleration = 5; // Default acceleration
    _last_update = 0;
    _left_forward = true;
    _right_forward = true;
}

void MotorController::begin() {
    pinMode(_left_pwm_pin, OUTPUT);
    pinMode(_left_dir_pin, OUTPUT);
    pinMode(_right_pwm_pin, OUTPUT);
    pinMode(_right_dir_pin, OUTPUT);
}

void MotorController::setLeftSpeed(int speed) {
    _target_left_speed = constrain(speed, 0, 255);
}

void MotorController::setRightSpeed(int speed) {
    _target_right_speed = constrain(speed, 0, 255);
}

void MotorController::setLeftDirection(bool forward) {
    _left_forward = forward;
    digitalWrite(_left_dir_pin, _left_forward ? HIGH : LOW);
}

void MotorController::setRightDirection(bool forward) {
    _right_forward = forward;
    digitalWrite(_right_dir_pin, _right_forward ? HIGH : LOW);
}

void MotorController::setLeftSpeedPercent(int percent) {
    percent = constrain(percent, -100, 100);
    if (percent >= 0) {
        setLeftDirection(true);
        _target_left_speed = map(percent, 0, 100, 0, 255);
    } else {
        setLeftDirection(false);
        _target_left_speed = map(abs(percent), 0, 100, 0, 255);
    }
}

void MotorController::setRightSpeedPercent(int percent) {
    percent = constrain(percent, -100, 100);
    if (percent >= 0) {
        setRightDirection(true);
        _target_right_speed = map(percent, 0, 100, 0, 255);
    } else {
        setRightDirection(false);
        _target_right_speed = map(abs(percent), 0, 100, 0, 255);
    }
}

void MotorController::setLeftAcceleration(int acceleration) {
    _left_acceleration = acceleration;
}

void MotorController::setRightAcceleration(int acceleration) {
    _right_acceleration = acceleration;
}

void MotorController::update() {
    unsigned long now = millis();
    if (now - _last_update > MOTOR_UPDATE_INTERVAL) { // Update every MOTOR_UPDATE_INTERVAL ms
        _last_update = now;

        // Left motor
        if (_current_left_speed < _target_left_speed) {
            _current_left_speed = min(_current_left_speed + _left_acceleration, _target_left_speed);
        } else if (_current_left_speed > _target_left_speed) {
            _current_left_speed = max(_current_left_speed - _left_acceleration, _target_left_speed);
        }

        // Right motor
        if (_current_right_speed < _target_right_speed) {
            _current_right_speed = min(_current_right_speed + _right_acceleration, _target_right_speed);
        } else if (_current_right_speed > _target_right_speed) {
            _current_right_speed = max(_current_right_speed - _right_acceleration, _target_right_speed);
        }

        analogWrite(_left_pwm_pin, _current_left_speed);
        analogWrite(_right_pwm_pin, _current_right_speed);
    }
}

int MotorController::getCurrentLeftSpeed() {
    int percent = map(_current_left_speed, 0, 255, 0, 100);
    return _left_forward ? percent : -percent;
}

int MotorController::getCurrentRightSpeed() {
    int percent = map(_current_right_speed, 0, 255, 0, 100);
    return _right_forward ? percent : -percent;
}

int MotorController::getLeftAcceleration() {
    return _left_acceleration;
}

int MotorController::getRightAcceleration() {
    return _right_acceleration;
}

int MotorController::getLeftDirection() {
    return _left_forward;
}

int MotorController::getRightDirection() {
    return _right_forward;
}