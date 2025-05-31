#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include <WiFi.hpp>
#include <global.hpp>
#include <config.hpp>

#ifdef __cplusplus
extern "C" {
#endif

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

extern WiFiClient wifiClient;
extern Arduino_MQTT_Client mqttClient;
extern ThingsBoard tb;

void TaskThingsBoard(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // MQTT_HPP
