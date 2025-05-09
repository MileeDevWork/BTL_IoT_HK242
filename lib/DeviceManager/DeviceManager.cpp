#include "DeviceManager.hpp"

DeviceManager::DeviceManager(const char* server, uint16_t port) 
    : thingsboardServer(server), thingsboardPort(port), deviceCount(0) {
}

DeviceManager::~DeviceManager() {
    // Cleanup resources
    for (int i = 0; i < deviceCount; i++) {
        delete devices[i].wifiClient;
        delete devices[i].mqttClient;
    }
}

int DeviceManager::addDevice(const char* name, const char* token, void (*callback)(char*, byte*, unsigned int)) {
    if (deviceCount >= MAX_DEVICES) return -1;
    
    int deviceIndex = deviceCount++;
    
    devices[deviceIndex].name = name;
    devices[deviceIndex].token = token;
    devices[deviceIndex].wifiClient = new WiFiClient();
    devices[deviceIndex].mqttClient = new PubSubClient(*devices[deviceIndex].wifiClient);
    devices[deviceIndex].connected = false;
    devices[deviceIndex].callback = callback;
    
    // Configure MQTT client
    devices[deviceIndex].mqttClient->setServer(thingsboardServer, thingsboardPort);
    if (callback != nullptr) {
        devices[deviceIndex].mqttClient->setCallback(callback);
    }
    
    return deviceIndex;
}

bool DeviceManager::connectDeviceToThingsBoard(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= deviceCount) return false;
    
    Device& device = devices[deviceIndex];
    String clientId = String(device.name) + "_Client";
    
    if (!device.mqttClient->connected()) {
        Serial.printf("Connecting %s to ThingsBoard... ", device.name);
        
        if (device.mqttClient->connect(clientId.c_str(), device.token, nullptr)) {
            device.connected = true;
            Serial.printf("Connected!\n");
            return true;
        } else {
            Serial.printf("Failed, rc=%d. Retrying...\n", device.mqttClient->state());
            return false;
        }
    }
    
    return true;
}

void DeviceManager::processDevices() {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].connected) {
            devices[i].mqttClient->loop();
        }
    }
}

bool DeviceManager::publishTelemetry(int deviceIndex, const char* payload) {
    if (deviceIndex < 0 || deviceIndex >= deviceCount || !devices[deviceIndex].connected) 
        return false;
    
    return devices[deviceIndex].mqttClient->publish("v1/devices/me/telemetry", payload);
}

bool DeviceManager::publishTelemetry(int deviceIndex, String& payload) {
    return publishTelemetry(deviceIndex, payload.c_str());
}

bool DeviceManager::isDeviceConnected(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= deviceCount) 
        return false;
    
    return devices[deviceIndex].connected;
}
