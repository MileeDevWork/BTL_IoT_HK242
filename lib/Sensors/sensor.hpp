#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include <mqtt.hpp>
#include "DHT20.h"
#include <MQ135.h>
#include "HCSR04.h"

#ifdef __cplusplus
extern "C" {
#endif
//
extern DHT20 dht20;
extern int airQuality;
extern MQ135 mq135_sensor;
extern UltraSonicDistanceSensor* ultrasonicSensor;
extern bool objectDetected;
extern bool motionDetected;

// Task functions
void readDHT20(void *pvParameters);
void readMQ135(void *pvParameters);
void ultrasonicTask(void *pvParameters);
void pirTask(void *pvParameters);




///
#ifdef __cplusplus
}
#endif

#endif