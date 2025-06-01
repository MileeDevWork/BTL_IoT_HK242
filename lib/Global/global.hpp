#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

extern float temperature;
extern float humidity;
extern SemaphoreHandle_t sensorDataMutex;
extern bool dhtReady;
extern int airQuality;
extern String category;
extern int peopleCount;
extern bool objectDetected;
extern bool motionDetected;
extern uint32_t previousDataSend;

// Area constant for density calculation
#define AREA_SQUARE_METERS 13000.0



///
#ifdef __cplusplus
}
#endif

#endif // GLOBAL_HPP