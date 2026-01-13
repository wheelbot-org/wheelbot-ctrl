#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <EspMQTTClient.h>
#include "config.h"
#include "MotorController.h"
#include "SensorManager.h"
#include "Steering.h"

extern MotorController* _motorController;
extern SensorManager* _sensorManager;
extern Steering* _steering;

void onConnectionEstablished();

namespace Communication {

void setup(MotorController* motorController, SensorManager* sensorManager, Steering* steering);
void loop();
void publish(const char* topic, const String& payload);
bool isConnected();
bool restartRequested();
void requestRestart();

} // namespace Communication

#endif // COMMUNICATION_H
