/*
  LuxSensor.h - Library for handling lux sensors
*/
#ifndef LuxSensor_h
#define LuxSensor_h

#include <Arduino_MachineControl.h>
#include <Ewma.h>
#include <EwmaT.h>

class LuxSensor
{
  Ewma adcFilter = Ewma(0.07);                  // EWMA filtering object - smooths out noise and jitter from pin readings.
                                                // EWMA.alpha: Smoothing factor for lux values. Lower is more smoothing but less responsive. Range of 0 - 1.0.
  public:
    LuxSensor(int pin);
    void readLux();

    boolean isReactiveChange();
    void luxSent(unsigned long timestampMillis);
    unsigned long getLastSentMillis();
    int getPin();
    unsigned int getLux();
    unsigned int getLuxReads();
    unsigned int getRawLux();
  private:
    int _pin;
    unsigned int  luxReads = 0;                  // How many times have we read
    unsigned int  rawLuxValue = 0;               // Raw lux value from sensor
    unsigned int  filteredLuxValue = 0;          // Filtered lux value       
    unsigned int  luxLastSent = 0;               // Last lux value to be published
    unsigned int  luxReactiveChangeRequired = 0; // Difference required between current and previously sent values to trigger a change
    unsigned int  luxReactiveDifference = 0;     // Difference between current and previously sent values
    float         luxReactiveThreshold = 0.1;    // Controls amount of change required between last sent and current lux value, before resending reactively. Range of 0 - 0.5
    bool          reactiveLuxSend = false;       // Whether or not a reactive send as been triggered
    unsigned long luxSentMillis = 0;             // Timestamp of when lux was last sent
};

#endif