#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include <mqtt.hpp>
#include "DHT.h"
#include <MQ135.h>

#ifdef __cplusplus
extern "C" {
#endif
//
#define DHTTYPE DHT11
extern DHT dht; 
extern MQ135 mq135_sensor;
void readDHT11(void *pvParameters);
void readMQ135(void *pvParameters);
String getAQICategory(int aqi);




///
#ifdef __cplusplus
}
#endif

#endif