#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include <WiFi.hpp>
#include <global.hpp>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cấu hình coreiot
constexpr char TOKEN[] = "OrMees1ToDgts03u5TsV";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

// Thời gian gửi dữ liệu lên coreiot
constexpr int16_t telemetrySendInterval = 5000U;

// Khai báo biến toàn cục
extern uint32_t previousDataSend;
extern WiFiClient wifiClient;
extern Arduino_MQTT_Client &mqttClient;  // dùng tham chiếu
extern ThingsBoard tb;

extern PubSubClient client;
extern const char* ledStateControlKey; // Sử dụng ledState làm key
// Biến lưu trữ trạng thái LED
extern volatile bool ledState;
//nhận trạng thái led từ task mqtt
extern QueueHandle_t ledStateQueue;

void TaskThingsBoard(void *pvParameters);
void callback(char* topic, byte* payload, unsigned int length);
void ledControlTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
