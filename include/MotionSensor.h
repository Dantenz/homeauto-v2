/*
  MotionSensor.h - Library for handling motion sensors
*/
#ifndef MotionSensor_h
#define MotionSensor_h

#include <Arduino_MachineControl.h>

class MotionSensor
{

  public:
    MotionSensor(int pin);
    bool readMotion();
    int getState(); 
    int getPin();

  private:
    int _pin;
    unsigned int  motionReads = 0;              // How many times have we read
    int           motionValue = 0;              // Motion value from sensor
    bool          motionState = 0;              // True if motion has been detected
};

#endif