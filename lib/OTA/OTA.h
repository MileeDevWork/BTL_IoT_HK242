#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Update.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khai báo biến toàn cục cho OTA
extern bool otaInProgress;
extern bool waitingForChunk;
extern int fw_size, chunk_size, chunks_received, offset;
extern String fw_title, fw_version, fw_checksum, fw_algo;
extern unsigned long lastRequestTime;
extern const unsigned long OTA_REQUEST_TIMEOUT;
extern byte* firmware_data;

// Cấu trúc thiết bị
struct Alldevice {
    const char* name;
    const char* token;
    WiFiClient* wifiClient;
    PubSubClient* mqttClient;
    bool connected;
    void (*callback)(char*, byte*, unsigned int);
};

// Thiết bị duy nhất
extern Alldevice device;
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;
extern const char* DEVICE_TOKEN;
extern bool deviceConnected;

// Khai báo hàm
int b64decode(char c);
size_t decode_base64(const char *input, uint8_t *output, size_t output_len);
void deviceCallback(char* topic, byte* payload, unsigned int length);
void initializeDevice();
bool connectDeviceToThingsBoard();
void requestFirmwareAttributes();
void startOtaProcess();
void otaTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif