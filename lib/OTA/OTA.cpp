#include "OTA.h"
#include <vector>

// Biến OTA toàn cục
bool otaInProgress = false;
bool waitingForChunk = false;
int fw_size = 0, chunk_size = 0, chunks_received = 0, offset = 0;
String fw_title = "", fw_version = "", fw_checksum = "", fw_algo = "sha256";

// Timeout cho OTA
unsigned long lastRequestTime = 0;
const unsigned long REQUEST_TIMEOUT = 5000; // 5 giây

// Mảng lưu thiết bị
Alldevice devices[10];
int deviceCount = 0;
int dhtDeviceIndex = -1;

// Hàm giải mã Base64
int b64decode(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t decode_base64(const char *input, uint8_t *output, size_t output_len) {
    size_t i = 0;
    int buffer = 0, bits = 0;
    while (*input && output_len) {
        int val = b64decode(*input++);
        if (val < 0) continue;
        buffer = (buffer << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            *output++ = (buffer >> bits) & 0xFF;
            output_len--;
            i++;
        }
    }
    return i;
}

// Hàm callback MQTT
void deviceCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("MQTT -> %s\n", topic);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    String topicStr = String(topic);

    // Xử lý thông tin firmware
    if (topicStr.indexOf("attributes") >= 0) {
        if (doc.containsKey("fw_title")) {
            fw_title = doc["fw_title"].as<String>();
            fw_version = doc["fw_version"].as<String>();
            fw_checksum = doc["fw_checksum"].as<String>();
            fw_algo = doc["fw_checksum_algorithm"].as<String>();
            fw_size = doc["fw_size"];
            chunk_size = doc["fw_chunk_size"];
            offset = 0;
            chunks_received = 0;

            otaInProgress = true;
            devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/attributes", "{\"fw_state\":\"INITIATED\"}");
        }
    }

    // Xử lý dữ liệu chunk firmware
    if (topicStr.indexOf("firmware/response") >= 0 && otaInProgress && waitingForChunk) {
        const char* b64data = doc["data"];
        int len = strlen(b64data);
        std::vector<uint8_t> decoded(len / 4 * 3);

        size_t actualLen = decode_base64(b64data, decoded.data(), decoded.size());
        if (Update.write(decoded.data(), actualLen) != actualLen) {
            Serial.println("Update write failed!");
            devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/attributes", "{\"fw_state\":\"FAILED\"}");
            otaInProgress = false;
            Update.abort();
            return;
        }

        offset += actualLen;
        chunks_received++;
        Serial.printf("Chunk %d received. Total offset: %d/%d\n", chunks_received, offset, fw_size);

        waitingForChunk = false;
        lastRequestTime = 0;

        if (offset >= fw_size) {
            if (Update.end(true)) {
                Serial.println("OTA Update Success!");
                devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/attributes", "{\"fw_state\":\"UPDATED\"}");
                vTaskDelay(pdMS_TO_TICKS(2000));
                ESP.restart();
            } else {
                Serial.println("Update end failed.");
                devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/attributes", "{\"fw_state\":\"FAILED\"}");
                otaInProgress = false;
            }
        }
    }
}

// Thêm thiết bị
int addDevice(const char* name, const char* token, void (*callback)(char*, byte*, unsigned int)) {
    if (deviceCount >= 10) return -1;
    int deviceIndex = deviceCount++;
    devices[deviceIndex].name = name;
    devices[deviceIndex].token = token;
    devices[deviceIndex].wifiClient = new WiFiClient();
    devices[deviceIndex].mqttClient = new PubSubClient(*devices[deviceIndex].wifiClient);
    devices[deviceIndex].connected = false;
    devices[deviceIndex].callback = callback;
    devices[deviceIndex].mqttClient->setServer("app.coreiot.io", 1883);
    devices[deviceIndex].mqttClient->setBufferSize(16384);
    if (callback != nullptr) {
        devices[deviceIndex].mqttClient->setCallback(callback);
    }
    return deviceIndex;
}

// Kết nối thiết bị với ThingsBoard
bool connectDeviceToThingsBoard(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= deviceCount) return false;
    Alldevice& device = devices[deviceIndex];
    String clientId = String(device.name) + "_Client";
    if (!device.mqttClient->connected()) {
        if (device.mqttClient->connect(clientId.c_str(), device.token, nullptr)) {
            device.connected = true;
            Serial.printf("Connected!\n");
            if (deviceIndex == dhtDeviceIndex) {
                device.mqttClient->subscribe("v1/devices/me/attributes");
                device.mqttClient->subscribe("v1/devices/me/firmware/response");
            }
            return true;
        } else {
            Serial.printf("Failed, rc=%d\n", device.mqttClient->state());
            return false;
        }
    }
    return true;
}

// Task OTA
void otaTask(void *pvParameters) {
    for (;;) {
        if (otaInProgress && !waitingForChunk && offset < fw_size) {
            if (!Update.begin(fw_size)) {
                Serial.println("Failed to begin OTA");
                devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/attributes", "{\"fw_state\":\"FAILED\"}");
                otaInProgress = false;
                continue;
            }

            String req = "{\"title\":\"" + fw_title + "\",\"version\":\"" + fw_version + "\",\"chunkSize\":" + String(chunk_size) + ",\"chunk\":\"" + String(offset) + "\"}";
            devices[dhtDeviceIndex].mqttClient->publish("v1/devices/me/firmware/request", req.c_str());
            waitingForChunk = true;
            lastRequestTime = millis();
            Serial.printf("Requested chunk at offset %d\n", offset);
        }

        if (waitingForChunk && millis() - lastRequestTime > REQUEST_TIMEOUT) {
            Serial.println("Timeout waiting for chunk. Retrying...");
            waitingForChunk = false;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}