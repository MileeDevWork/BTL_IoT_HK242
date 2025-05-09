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
extern bool dhtReady;
extern bool motionDetected;

//GPIO
#define DHTPIN 8 // D5, DHT sensor
#define MQ135_PIN 1 // MQ135
#define SONIC_TRIGGER GPIO_NUM_6 // HC-SR04 Trigger pin
#define SONIC_ECHO GPIO_NUM_7    // HC-SR04 Echo pin
#define LED_PIN 48               // LED pin
#define PIR_PIN GPIO_NUM_18              // PIR motion sensor pin
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12


///
#ifdef __cplusplus
}
#endif

#endif