#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include "DHT.h"

#ifdef __cplusplus
extern "C" {
#endif
//
#define DHTTYPE DHT11
extern DHT dht; 
void readDHT11(void *pvParameters);



///
#ifdef __cplusplus
}
#endif

#endif