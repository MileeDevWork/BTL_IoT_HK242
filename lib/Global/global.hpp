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

// Chân kết nối cảm biến
#define DHTPIN 8         // Chân DHT11
#define MQ135_PIN 1      // Chân MQ135
#define TRIG_ENTER 21    // Chân TRIG siêu âm
#define ECHO_ENTER 20    // Chân ECHO siêu âm
#define pirPin 18        // Chân PIR đầu tiên
#define PIR_PIN2 19      // Chân PIR thứ hai

#ifdef __cplusplus
}
#endif

#endif // GLOBAL_HPP