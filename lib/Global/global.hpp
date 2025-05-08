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

//GPIO
#define DHTPIN 8 // D5


///
#ifdef __cplusplus
}
#endif

#endif