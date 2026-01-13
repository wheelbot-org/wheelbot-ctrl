#ifndef STEERING_H
#define STEERING_H

#include <Arduino.h>
#include <Servo.h>

class Steering {
public:
    Steering();
    void begin();
    void setAngle(int angle);
    int getAngle();
    void setAcceleration(int acceleration);
    int getAcceleration();
    void update();

private:
    Servo _servo;
    int _current_angle;
    int _target_angle;
    int _acceleration;
    unsigned long _last_update;
};

#endif // STEERING_H
