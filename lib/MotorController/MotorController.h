#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "config.h"


class MotorController {
public:
    MotorController(int left_pwm_pin, int left_dir_pin, int right_pwm_pin, int right_dir_pin);
    void begin();
    void setLeftSpeed(int speed);
    void setRightSpeed(int speed);
    void setLeftDirection(bool forward);
    void setRightDirection(bool forward);
    void setLeftSpeedPercent(int percent);
    void setRightSpeedPercent(int percent);
    void setLeftAcceleration(int acceleration);
    void setRightAcceleration(int acceleration);
    void update();
    int getCurrentLeftSpeed();
    int getCurrentRightSpeed();
    int getLeftAcceleration();
    int getRightAcceleration();
    int getLeftDirection();
    int getRightDirection();

private:
    int _left_pwm_pin;
    int _left_dir_pin;
    int _right_pwm_pin;
    int _right_dir_pin;

    int _current_left_speed;
    int _current_right_speed;
    int _target_left_speed;
    int _target_right_speed;
    int _left_acceleration;
    int _right_acceleration;
    unsigned long _last_update;
    bool _left_forward;
    bool _right_forward;
    bool _target_left_forward;
    bool _target_right_forward;
    bool _left_direction_change_pending;
    bool _right_direction_change_pending;
};

#endif // MOTOR_CONTROLLER_H
