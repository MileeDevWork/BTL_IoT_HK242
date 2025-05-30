#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
//

extern float temperature;
extern float humidity;
extern SemaphoreHandle_t sensorDataMutex;
extern bool dhtReady;
extern int airQuality;
extern String category;
extern int peopleCount;
extern bool objectDetected;
extern bool motionDetected;



//GPIO
#define DHTPIN 8 // D5, DHT11
#define MQ135_PIN 1 // MQ135
#define TRIG_ENTER 21 //D10
#define ECHO_ENTER 18 //D9

#define pirPin 18 //D9
#define PIR_PIN2 19 //D10



///
#ifdef __cplusplus
}
#endif

#endif // GLOBAL_HPP