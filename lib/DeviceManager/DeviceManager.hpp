#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define MAX_DEVICES 10

struct Device {
    const char* name;
    const char* token;
    WiFiClient* wifiClient;
    PubSubClient* mqttClient;
    bool connected;
    void (*callback)(char*, byte*, unsigned int);
};

class DeviceManager {
public:
    DeviceManager(const char* server, uint16_t port);
    ~DeviceManager();
    
    int addDevice(const char* name, const char* token, void (*callback)(char*, byte*, unsigned int) = nullptr);
    bool connectDeviceToThingsBoard(int deviceIndex);
    void processDevices();
    bool publishTelemetry(int deviceIndex, const char* payload);
    bool publishTelemetry(int deviceIndex, String& payload);
    bool isDeviceConnected(int deviceIndex);
    
private:
    const char* thingsboardServer;
    uint16_t thingsboardPort;
    Device devices[MAX_DEVICES];
    int deviceCount;
};

#endif // DEVICE_MANAGER_HPP
