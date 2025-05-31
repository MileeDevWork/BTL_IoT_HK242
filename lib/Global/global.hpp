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

//GPIO
#define DHTPIN 8 // D5, DHT11
#define MQ135_PIN 1 // MQ135
#define TRIG_ENTER 21 //D10
#define ECHO_ENTER 18 //D9

#define pirPinIn 18 //D9
#define pirPinOut 10 //D7
#define AREA_SQUARE_METERS 13000.0



///
#ifdef __cplusplus
}
#endif

#endif // GLOBAL_HPP