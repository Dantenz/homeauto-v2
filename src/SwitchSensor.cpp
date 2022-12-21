/*
  SwitchSensor.cpp - Library for handling basic switches
*/

#include <Arduino_MachineControl.h>
#include <SwitchSensor.h>

using namespace machinecontrol;

SwitchSensor::SwitchSensor(int pin)
{
    _pin = pin;
}

bool SwitchSensor::readSwitch()
{
  currentMillis = millis();

  bool returnValue = false;

  // Ensure cooldown period between state changes to prevent rapid changes
  if (currentMillis - prevMillis >= cooldownFrequency) {
    switchValue = digital_inputs.read(_pin);

    // Return true on state change and record the current state
    if (switchValue == 1 && switchState == 0) {
      Serial.println(switchValue);
      switchState = 1;

      prevMillis = currentMillis;
      
      returnValue = true;
    } else if (switchValue == 0 && switchState == 1) {
      Serial.println(switchValue);
      switchState = 0;

      prevMillis = currentMillis;

      returnValue = true;
    }
  }

  return returnValue;
}

int SwitchSensor::getState()
{
    return switchState;
}

int SwitchSensor::getPin()
{
    return _pin;
}
