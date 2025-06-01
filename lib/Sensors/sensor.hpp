#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include <mqtt.hpp>
#include <config.hpp>
#include "DHT.h"
#include <MQ135.h>
#include <HCSR04.h>
#include "DHT20.h"

#ifdef __cplusplus
extern "C" {
#endif
//
#define DHTTYPE DHT11
extern DHT20 dht20;
extern DHT* dht;  // Now a pointer for dynamic initialization
extern MQ135* mq135_sensor;  // Now a pointer for dynamic initialization
//biến gán để test hàm mật độ dân số
extern bool objectDetected;
extern bool motionDetected;
extern UltraSonicDistanceSensor* ultrasonicSensor[10];
extern bool CarDetected[10];

void readDHT20(void *pvParameters);
void readDHT11(void *pvParameters);
void readMQ135(void *pvParameters);
String getAQICategory(int aqi);
void initUltrasonicSensors();  // Added function declaration
void peopleCountingTask(void *pvParameters);
void ultrasonicTask(void *pvParameters);
void carslotTask(void *pvParameters);
void pirTask(void *pvParameters);
void rfidTask(void *pvParameters);

///
#ifdef __cplusplus
}
#endif

#endif