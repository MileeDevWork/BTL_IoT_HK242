#ifndef MQTT_HPP
#define MQTT_HPP

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <global.hpp>
#include <config.hpp>
#include <ThingsBoard.h>
#include <control.hpp>
#ifdef __cplusplus
extern "C" {
#endif

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

#define LED_PIN 48
extern QueueHandle_t ledStateQueue;
extern const char* ledStateControlKey;

void TaskThingsBoard(void *pvParameters);
void ledControlTask(void *pvParameters);
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool reconnect();

#ifdef __cplusplus
}
#endif

#endif // MQTT_HPP