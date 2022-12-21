/*
  LuxSensor.cpp - Library for handling lux sensors
*/

#include <Arduino_MachineControl.h>
#include <Ewma.h>
#include <EwmaT.h>
#include <LuxSensor.h>

using namespace machinecontrol;

LuxSensor::LuxSensor(int pin)
{
    _pin = pin;
}

void LuxSensor::readLux()
{
    rawLuxValue = analog_in.read(_pin); // Dummy read to allow capacitor to settle when multiplexing
    rawLuxValue = analog_in.read(_pin);

    luxReads++;
    
    filteredLuxValue = round(adcFilter.filter(rawLuxValue));

    reactiveLuxSend = false;

    if (filteredLuxValue >= luxLastSent) luxReactiveDifference = filteredLuxValue - luxLastSent;
    else                                 luxReactiveDifference = luxLastSent - filteredLuxValue;
    
    // Only reactive send if the value has actually changed
    if (luxReactiveDifference >= 1) {
      // Check if the difference is enough to trigger a reactive change
      luxReactiveChangeRequired = floor(filteredLuxValue * luxReactiveThreshold) + 1;

      if (luxReactiveDifference >= luxReactiveChangeRequired) reactiveLuxSend = true;
    }
}

int LuxSensor::getPin()
{
    return _pin;
}

unsigned int LuxSensor::getLux()
{
    return filteredLuxValue;
}

unsigned long LuxSensor::getLastSentMillis()
{
    return luxSentMillis;
}

boolean LuxSensor::isReactiveChange()
{
    return reactiveLuxSend;
}

void LuxSensor::luxSent(unsigned long timestampMillis)
{
    luxLastSent = filteredLuxValue;
    luxSentMillis = timestampMillis;
}

unsigned int LuxSensor::getLuxReads()
{
    return luxReads;
}

unsigned int LuxSensor::getRawLux()
{
    return rawLuxValue;
}