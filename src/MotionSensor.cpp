/*
  MotionSensor.cpp - Library for handling motion sensors
*/

#include <Arduino_MachineControl.h>
#include <MotionSensor.h>

using namespace machinecontrol;

MotionSensor::MotionSensor(int pin)
{
    _pin = pin;
}

bool MotionSensor::readMotion()
{
  motionValue = digital_programmables.read(_pin);

  // Sensor has a built-in 1 sec leadtime between triggering on and switching off
  // Return true on state change and record the current state
  if (motionValue == 1 && motionState == 0) {
    Serial.println(motionValue);
    motionState = 1;
    
    return true;
  } else if (motionValue == 0 && motionState == 1) {
    Serial.println(motionValue);
    motionState = 0;

    return true;
  } else {
    return false;
  }
}

int MotionSensor::getState()
{
    return motionState;
}

int MotionSensor::getPin()
{
    return _pin;
}
