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

//GPIO
#define DHTPIN 8 // D5, DHT11
#define MQ135_PIN 1 // MQ135



///
#ifdef __cplusplus
}
#endif

#endif