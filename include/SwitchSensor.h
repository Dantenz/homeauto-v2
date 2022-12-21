/*
  SwitchSensor.h - Library for handling basic switches
*/
#ifndef SwitchSensor_h
#define SwitchSensor_h

#include <Arduino_MachineControl.h>

class SwitchSensor
{

  public:
    SwitchSensor(int pin);
    bool readSwitch();
    int getState(); 
    int getPin();

  private:
    int _pin;
    unsigned long         prevMillis = 0;              // Millis value at last state change
    unsigned long  cooldownFrequency = 1000;           // How long to wait (in millis) before updating state - prevents rapid changes
    unsigned long      currentMillis = 0;
    unsigned int         switchReads = 0;              // How many times have we read
    int                  switchValue = 0;
    bool                 switchState = 0;              // True if switch is closed
};

#endif