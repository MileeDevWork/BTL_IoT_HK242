#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <global.hpp>
#include <mqtt.hpp>
#include <config.hpp>
#include "DHT.h"
#include <MQ135.h>
#include <HCSR04.h>
#include "DHT20.h"
#include <SPI.h>
#include <MFRC522.h>

#ifdef __cplusplus
extern "C" {
#endif
//
#define DHTTYPE DHT11
extern DHT20 dht20;
extern DHT* dht;  //pointer for dynamic initialization
extern MQ135* mq135_sensor;  // pointer for dynamic initialization
extern MFRC522* mfrc522;  // RFID sensor pointer
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
void initRFIDSensor();  // RFID initialization function

// Parking management constants
#define PARKING_DETECTION_THRESHOLD 10.0f
#define PARKING_STATS_UPDATE_INTERVAL 5000
#define PARKING_INITIAL_DELAY 5000

// Parking management functions
void updateParkingStats();
void sendParkingDataToThingsBoard();
void initParkingStats();

///
#ifdef __cplusplus
}
#endif

#endif