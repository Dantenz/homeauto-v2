#include <Arduino_MachineControl.h>
#include <PortentaEthernet.h>
#include <PubSubClient.h>
#include <Ewma.h>
#include <EwmaT.h>
#include <LuxSensor.h>
#include <MotionSensor.h>
#include <SwitchSensor.h>

using namespace machinecontrol;

/*
  Build Configuration 
  
  "Sensitive" information such as logins and passwords should be defined at build-time if 
  building with platformio, by setting the build_flags parameter in the target build environment.

  Otherwise, just change the below values to your preferred ones and run.

*/

#ifndef MQTT_SERVER_ADDRESS
#define MQTT_SERVER_ADDRESS "192.168.0.1"
#endif

#ifndef MQTT_SERVER_USERNAME
#define MQTT_SERVER_USERNAME "admin"
#endif

#ifndef MQTT_SERVER_PASSWORD
#define MQTT_SERVER_PASSWORD "password"
#endif

// Arduino IP Address
#if !defined(IP1) || !defined(IP2) || !defined(IP3) || !defined(IP4)
#define IP1 192
#define IP2 168
#define IP3 0
#define IP4 20
#endif

#ifndef DEVICE_NAME
#define DEVICE_NAME "arduino"
#endif

/*
  End Build Configuration 
*/


/*
  Parameters

  The following can be changed to alter the behaviour of the code or accomodate your physical setup
*/

// Sensor
unsigned int  mainLoopFrequency = 1;                    // How long to wait between main loop iterations in ms
unsigned int  luxPollFrequency = 250;                   // How often to read lux in ms
unsigned long luxPublishFrequency = 60000;              // How often to publish lux in ms
float         luxReactiveThreshold = 0.1;               // Controls amount of change required between last sent and current lux value, before resending reactively. Range of 0 - 0.5.
float         filterAlpha = 0.07;                       // Smoothing factor for lux values. Lower is more smoothing but less responsive. Range of 0 - 1.0.

LuxSensor     luxSensors[] = {LuxSensor(0), 
                              LuxSensor(1), 
                              LuxSensor(2)};

MotionSensor  motionSensors[] = {MotionSensor(IO_READ_CH_PIN_00),
                                 MotionSensor(IO_READ_CH_PIN_01),
                                 MotionSensor(IO_READ_CH_PIN_02),
                                 MotionSensor(IO_READ_CH_PIN_03),
                                 MotionSensor(IO_READ_CH_PIN_04),
                                 MotionSensor(IO_READ_CH_PIN_05),
                                 MotionSensor(IO_READ_CH_PIN_06),
                                 MotionSensor(IO_READ_CH_PIN_07),
                                 MotionSensor(IO_READ_CH_PIN_08),
                                 MotionSensor(IO_READ_CH_PIN_09),
                                 MotionSensor(IO_READ_CH_PIN_10),
                                 MotionSensor(IO_READ_CH_PIN_11)};


SwitchSensor switchSensors[] = {SwitchSensor(DIN_READ_CH_PIN_00),
                                SwitchSensor(DIN_READ_CH_PIN_01),
                                SwitchSensor(DIN_READ_CH_PIN_02),
                                SwitchSensor(DIN_READ_CH_PIN_03),
                                SwitchSensor(DIN_READ_CH_PIN_04),
                                SwitchSensor(DIN_READ_CH_PIN_05),
                                SwitchSensor(DIN_READ_CH_PIN_06),
                                SwitchSensor(DIN_READ_CH_PIN_07)};


// MQTT
unsigned int mqttReconnectFrequency = 5000;             // How long to wait between reconnection attempts in ms

/*
  End Parameters
*/


// MQTT
EthernetClient ethClient;
IPAddress      ip(IP1, IP2, IP3, IP4);      // IP address of the arduino
byte           mac[] = { 0xFE, 0xED, 0xDE, 0xAD, 0xBE, 0xEF };
unsigned int   mqttReconnectMillis = 0;
PubSubClient   mqttClient(ethClient);
const char*    mqttServerAddress = MQTT_SERVER_ADDRESS;      // IP address of mqtt server
const char*    mqttServerUsername = MQTT_SERVER_USERNAME;    // Username used to connect to mqtt server
const char*    mqttServerPassword = MQTT_SERVER_PASSWORD;    // Password used to connect to mqtt server
String         topic = "";

// General
unsigned long currentMillis = 0;            // Updates baseline for all millisecond comparisons
unsigned long globalLoopPreviousMillis = 0; // Compared between currentMillis for mainLoopFrequency calculation
char          charBuffer[50];               // Stores converted vals
char          topicBuffer[50];              // Stores converted vals

// debugmode
unsigned long debugPreviousMillis = 0;
unsigned int  luxReads = 0;                 // How many times have we read

// Lux
unsigned long luxPollPreviousMillis = 0;    // Compared between currentMillis for luxPollFrequency calculation

/*
  End General Vars
*/


void readLux();
void readMotion();
void readSwitch();
void mqttReconnect();
bool publishToTopic(char* topic, char* payload);
bool isTopic(char* topicReceived, char* topicToMatch);
char* buildTopic(String t1, String t2 = "", String t3 = "", String t4 = "", String t5 = "");
char* intToChar(int intValue);
char* floatToChar(float floatValue);
char* longToChar(long longValue);
String byteToString(byte* payload, unsigned int length);
float clamp(float d, float min, float max);


float res_divider = 0.28057;
float reference = 3.3;

void setup() {
  Serial.begin(9600);

  // Analogue setup
  analogReadResolution(16);
  analog_in.set0_10V();

  // Digital In setup
  Wire.begin();

  if (!digital_inputs.init()) {
    Serial.println("Digital input initialization fail!!");
  }

  // Programmable IO setup
  if (!digital_programmables.init()) {
    Serial.println("GPIO expander initialization fail!!");
  }

  Serial.println("GPIO initialization complete");
  digital_programmables.setLatch();


  mqttClient.setServer(mqttServerAddress, 1883);

  Ethernet.begin(mac, ip);

  sprintf(charBuffer, "Starting ethernet with IP %d.%d.%d.%d", IP1, IP2, IP3, IP4);
  Serial.println(charBuffer);

   // Don't wait for reconnection timmer - connect immediately
  mqttReconnectMillis = millis() - mqttReconnectFrequency;
  mqttReconnect();

  Serial.println("Completed setup");
}

void loop() {
  currentMillis = millis();
  
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  
  if (currentMillis - globalLoopPreviousMillis >= mainLoopFrequency) {
    globalLoopPreviousMillis = currentMillis;
  
    readLux();
    readMotion();
    readSwitch();
  }
}

void readLux() {
  // We will send an update every "luxPublishFrequency" ms.
  // However, if a value varies from the last published by a certain degree (i.e. a light turning on or a curtain closing)
  // then we'll send it immediately. Theoretically we could send a message every "luxPublishFrequency", but this is highly
  // unlikely (unless you have a strobe light, in which case you can lower the adcFilter alpha) 

  if (currentMillis - luxPollPreviousMillis >= luxPollFrequency) {
    luxPollPreviousMillis = currentMillis;

    for(LuxSensor & currentLuxSensor : luxSensors) {
        currentLuxSensor.readLux();

        // Should we send the latest average?
        if (currentLuxSensor.isReactiveChange() || currentMillis - currentLuxSensor.getLastSentMillis() >= luxPublishFrequency) {
          // Lux has either adaptively shifted beyond the last sent value, or the timer has ticked over
          Serial.println("Lux value changed");
          publishToTopic(buildTopic(DEVICE_NAME, DEVICE_ID, "lux", String(currentLuxSensor.getPin())), intToChar(currentLuxSensor.getLux()));
          
          // Update with current timestamp
          currentLuxSensor.luxSent(currentMillis);
        }
    }
  }
}

void readMotion() {
  // This allows us to name the mqtt topic use sequential numbering
  // Relying on the pin numbers gives non-logical sequences
  int index = 0;

  // Loop through all motion sensors and send state updates if needed
  for(MotionSensor & currentMotionSensor : motionSensors) {
    if (currentMotionSensor.readMotion()) {
      Serial.println("Motion state changed");
      publishToTopic(buildTopic(DEVICE_NAME, DEVICE_ID, "motion", String(index)), intToChar(currentMotionSensor.getState()));
    }
    index++;
  }
}

void readSwitch() { 
  // This allows us to name the mqtt topic use sequential numbering
  // Relying on the pin numbers gives non-logical sequences
  int index = 0;

  // Loop through all switches and send state updates if needed
  for(SwitchSensor & currentSwitchSensor : switchSensors) {
    if (currentSwitchSensor.readSwitch()) {
      Serial.println("Switch state changed");
      publishToTopic(buildTopic(DEVICE_NAME, DEVICE_ID, "switch", String(index)), intToChar(currentSwitchSensor.getState()));
    }
    index++;
  }
}

void mqttReconnect() {
  // Try to reconnect if we're not connected, but ensure there's a delay and the sensors continue to poll
  if(currentMillis - mqttReconnectMillis >= mqttReconnectFrequency) {
    mqttReconnectMillis = currentMillis;
    
    Serial.print("Attempting MQTT connection to "); Serial.print(mqttServerAddress); Serial.print("...");
    // Attempt to connect
    if (mqttClient.connect("arduinoClient", mqttServerUsername, mqttServerPassword)) {
      Serial.println("connected");

    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
}

bool publishToTopic(char* topic, char* payload) {
  bool returnVal = true;

  if(!mqttClient.publish(topic, payload)) {
    returnVal = false;
    Serial.println("Failed to publish payload");
  }

  return returnVal;
}

bool isTopic(char* topicReceived, char* topicToMatch) {
  // Abstract the backwards strcmp and make life a bit easier
  return !strcmp(topicReceived, topicToMatch);
}

char* buildTopic(String t1, String t2, String t3, String t4, String t5) {
    String outStr = t1;

    if (t2.length() > 0)  outStr += "/" + t2;
    if (t3.length() > 0)  outStr += "/" + t3;
    if (t4.length() > 0)  outStr += "/" + t4;
    if (t5.length() > 0)  outStr += "/" + t5;

    outStr.toCharArray(topicBuffer, outStr.length()+1);

    return topicBuffer;
}

char* intToChar(int intValue) {
  String outStr = String(intValue);
  outStr.toCharArray(charBuffer, outStr.length()+1);
  return charBuffer;
}

String intToString(int intValue) {
  return String(intValue);
}

char* floatToChar(float floatValue) {
  String outStr = String(floatValue);
  outStr.toCharArray(charBuffer, outStr.length()+1);
  return charBuffer;
}

char* longToChar(long longValue) {
  String outStr = String(longValue);
  outStr.toCharArray(charBuffer, outStr.length()+1);
  return charBuffer;
}

String byteToString(byte* payload, unsigned int length) {
  unsigned int i; 
  String outStr = ""; 
  for (i = 0; i < length; i++) { 
      outStr = outStr + char(payload[i]); 
  } 
  return outStr; 
}

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}
