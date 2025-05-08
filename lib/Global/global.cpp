#include <global.hpp>

float temperature = NAN;
float humidity = NAN;
SemaphoreHandle_t sensorDataMutex = NULL;