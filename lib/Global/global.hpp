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

// Pin definitions moved to config.hpp for dynamic device-specific configuration

#ifdef __cplusplus
}
#endif

#endif // GLOBAL_HPP