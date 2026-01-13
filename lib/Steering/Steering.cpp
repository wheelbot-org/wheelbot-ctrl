#include <Arduino.h>
#include "config.h"
#include "Steering.h"

Steering::Steering() {
    _current_angle = 90;
    _target_angle = 90;
    _acceleration = 1; // Default acceleration
    _last_update = 0;
}

void Steering::begin() {
    _servo.attach(STEERING_WHEEL);
    _servo.write(_current_angle);
}

void Steering::setAngle(int angle) {
    _target_angle = constrain(angle, 0, 180);
}

int Steering::getAngle() {
    return _current_angle;
}

void Steering::setAcceleration(int acceleration) {
    _acceleration = acceleration;
}

int Steering::getAcceleration() {
    return _acceleration;
}

void Steering::update() {
    unsigned long now = millis();
    if (now - _last_update > STEERING_UPDATE_INTERVAL) { // Update every STEERING_UPDATE_INTERVAL ms
        _last_update = now;

        if (_current_angle < _target_angle) {
            _current_angle = min(_current_angle + _acceleration, _target_angle);
        } else if (_current_angle > _target_angle) {
            _current_angle = max(_current_angle - _acceleration, _target_angle);
        }

        _servo.write(_current_angle);
    }
}
