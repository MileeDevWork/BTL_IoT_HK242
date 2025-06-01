#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include <mqtt.hpp>
#include "DHT.h"
#include <MQ135.h>
#include <HCSR04.h>


#ifdef __cplusplus
extern "C" {
#endif
//
#define DHTTYPE DHT11
extern DHT dht; 
extern MQ135 mq135_sensor;
//biến gán để test hàm mật độ dân số
extern UltraSonicDistanceSensor* ultrasonicSensor;
extern bool objectDetected;


void readDHT11(void *pvParameters);
void readMQ135(void *pvParameters);
String getAQICategory(int aqi);
void peopleCountingTask(void *pvParameters);
void ultrasonicTask(void *pvParameters);



///
#ifdef __cplusplus
}
#endif

#endif