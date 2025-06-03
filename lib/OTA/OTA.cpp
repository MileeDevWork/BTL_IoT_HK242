#include "OTA.h"
#include <vector>

// Biến OTA toàn cục
bool otaInProgress = false;
bool waitingForChunk = false;
int fw_size = 0, chunk_size = 0, chunks_received = 0, offset = 0;
String fw_title = "", fw_version = "", fw_checksum = "", fw_algo = "sha256";
byte* firmware_data = nullptr;

// Timeout cho OTA
unsigned long lastRequestTime = 0;
const unsigned long OTA_REQUEST_TIMEOUT = 60000; // 60 giây

// Thiết bị duy nhất với token cố định
Alldevice device;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char* DEVICE_TOKEN = "EcmicQgZr1doHCGY5tb2";
bool deviceConnected = false;

// Biến toàn cục cho firmware request ID
static int currentFirmwareRequestId = 0;

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
    Serial.printf("MQTT callback -> Topic: %s, Length: %d\n", topic, length);
    
    // Log payload content for debugging (first 64 bytes max)
    Serial.print("Payload: ");
    for (unsigned int i = 0; i < min(length, (unsigned int)64); i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Chuyển đổi payload thành JSON nếu định dạng phù hợp
    char payloadStr[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payloadStr);
    if (error && strncmp(topic, "v2/fw/response", 14) != 0) {  // Skip JSON parsing for binary chunk data
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }
    
    String topicStr = String(topic);
    // Kiểm tra nếu có thông tin firmware từ response của attributes request
    if (topicStr.indexOf("attributes/response") >= 0) {
        Serial.println("Received attribute response");
        if (doc.containsKey("shared")) {
            JsonObject shared = doc["shared"];
            Serial.print("Shared attributes: ");
            serializeJson(shared, Serial);
            Serial.println();
            
            if (shared.containsKey("fw_title") && shared.containsKey("fw_version")) {
                fw_title = shared["fw_title"].as<String>();
                fw_version = shared["fw_version"].as<String>();
                fw_checksum = shared["fw_checksum"].as<String>();
                fw_algo = shared.containsKey("fw_checksum_algorithm") ? 
                          shared["fw_checksum_algorithm"].as<String>() : "sha256";
                fw_size = shared["fw_size"];
                chunk_size = shared.containsKey("fw_chunk_size") ? shared["fw_chunk_size"].as<int>() : 0;
                
                // Đặt kích thước chunk mặc định nếu không được cung cấp hoặc không hợp lệ
                if (chunk_size <= 0) {
                    chunk_size = 8192;  // Kích thước chunk mặc định
                }
                
                Serial.printf("Detected firmware from attributes response: %s v%s (size: %d bytes, chunk_size: %d)\n", 
                            fw_title.c_str(), fw_version.c_str(), fw_size, chunk_size);
                
                // Reset các biến trạng thái và bắt đầu OTA
                startOtaProcess();
            } else {
                Serial.println("No firmware info in shared attributes");
            }
        } else {
            Serial.println("No shared attributes in response");
        }
    }
    // Xử lý thông tin firmware từ thông báo attributes
    else if (topicStr.indexOf("attributes") >= 0) {
        if (doc.containsKey("fw_title")) {
            fw_title = doc["fw_title"].as<String>();
            fw_version = doc["fw_version"].as<String>();
            fw_checksum = doc["fw_checksum"].as<String>();
            fw_algo = doc.containsKey("fw_checksum_algorithm") ? 
                      doc["fw_checksum_algorithm"].as<String>() : "sha256";
            fw_size = doc["fw_size"];
            chunk_size = doc.containsKey("fw_chunk_size") ? doc["fw_chunk_size"].as<int>() : 0;
            
            // Đặt kích thước chunk mặc định nếu không được cung cấp hoặc không hợp lệ
            if (chunk_size <= 0) {
                chunk_size = 4096;  // Kích thước chunk mặc định
            }
            
            // Bắt đầu quá trình OTA
            startOtaProcess();
        }
    }
    // Xử lý dữ liệu chunk firmware
    else if (topicStr.indexOf("v2/fw/response") >= 0 && otaInProgress) {
        Serial.println("Received firmware chunk response");
        Serial.printf("Topic: %s, Length: %d bytes\n", topic, length);
        
        // Kiểm tra xem Update đã được khởi tạo chưa
        if (!Update.isRunning()) {
            Serial.println("Update is not initialized! Initializing now...");            if (!Update.begin(fw_size, U_FLASH)) {
                Serial.println("Failed to begin OTA: " + String(Update.errorString()));
                mqttClient.publish("v1/devices/me/attributes", 
                    "{\"fw_state\":\"FAILED\",\"fw_error\":\"Failed to initialize update\"}");
                otaInProgress = false;
                return;
            }
            Serial.println("OTA update initialized successfully");
        }
        
        // Ghi dữ liệu firmware vào flash
        if (Update.write(payload, length) != length) {
            Serial.printf("Update write failed: %s\n", Update.errorString());
            mqttClient.publish("v1/devices/me/attributes", 
                "{\"fw_state\":\"FAILED\",\"fw_error\":\"Failed to write firmware chunk\"}");
            otaInProgress = false;
            Update.abort();
            return;
        }

        offset += length;
        chunks_received++;
        Serial.printf("Chunk %d received. Total offset: %d/%d (%.1f%%)\n", 
            chunks_received, offset, fw_size, (float)offset * 100 / fw_size);

        waitingForChunk = false;
        lastRequestTime = 0;
        
        // Kiểm tra nếu đã nhận đủ dữ liệu firmware
        if (offset >= fw_size) {
            Serial.println("All chunks received, finalizing update...");
            mqttClient.publish("v1/devices/me/attributes", "{\"fw_state\":\"DOWNLOADED\"}");
            
            // Hoàn thành quá trình update
            if (Update.end(true)) {
                Serial.println("OTA Update Success! Rebooting...");
                mqttClient.publish("v1/devices/me/attributes", "{\"fw_state\":\"UPDATED\"}");
                vTaskDelay(pdMS_TO_TICKS(2000));
                ESP.restart();
            } else {
                Serial.printf("Update end failed: %s\n", Update.errorString());
                mqttClient.publish("v1/devices/me/attributes", 
                    "{\"fw_state\":\"FAILED\",\"fw_error\":\"Firmware verification failed\"}");
                otaInProgress = false;
            }
        }
    }
}

// Bắt đầu quá trình OTA
void startOtaProcess() {
    // Reset các biến trạng thái cho cập nhật mới
    offset = 0;
    chunks_received = 0;
    
    // Kiểm tra xem dung lượng đủ cho update không
    if (fw_size > 0 && ESP.getFreeSketchSpace() < fw_size) {
        Serial.printf("Not enough space for firmware update. Need %d bytes, available %d bytes\n", 
                    fw_size, ESP.getFreeSketchSpace());
        mqttClient.publish("v1/devices/me/attributes", 
                    "{\"fw_state\":\"FAILED\",\"fw_error\":\"Not enough space\"}");
        return;
    }

    currentFirmwareRequestId++;  // Tăng ID cho OTA mới
    Serial.printf("Starting OTA: %s v%s (size: %d bytes, chunk_size: %d)\n", 
                fw_title.c_str(), fw_version.c_str(), fw_size, chunk_size);
    
    otaInProgress = true;
    
    // Đảm bảo Update không đang chạy từ lần OTA trước
    if (Update.isRunning()) {
        Update.abort();
    }
    
    mqttClient.publish("v1/devices/me/attributes", "{\"fw_state\":\"INITIATED\"}");
}

// Khởi tạo thiết bị duy nhất
void initializeDevice() {
    device.name = "DHT_Device";
    device.token = DEVICE_TOKEN;
    device.wifiClient = &wifiClient;
    device.mqttClient = &mqttClient;
    device.connected = false;
    device.callback = deviceCallback;
    
    mqttClient.setServer("app.coreiot.io", 1883);
    mqttClient.setBufferSize(16384); // Tăng kích thước buffer cho chunk lớn hơn
    mqttClient.setCallback(deviceCallback);
}

// Kết nối thiết bị với ThingsBoard
bool connectDeviceToThingsBoard() {
    String clientId = String(device.name) + "_Client";
    if (!mqttClient.connected()) {
        if (mqttClient.connect(clientId.c_str(), DEVICE_TOKEN, nullptr)) {
            deviceConnected = true;
            Serial.printf("Connected to ThingsBoard! Client ID: %s\n", clientId.c_str());
            
            // Đăng ký topics
            mqttClient.subscribe("v1/devices/me/attributes");
            Serial.println("Subscribed to: v1/devices/me/attributes");
            
            mqttClient.subscribe("v1/devices/me/attributes/response/+");
            Serial.println("Subscribed to: v1/devices/me/attributes/response/+");
            
            mqttClient.subscribe("v2/fw/response/#");
            Serial.println("Subscribed to: v2/fw/response/#");
            
            // Gửi yêu cầu kiểm tra firmware mới ngay sau khi kết nối
            Serial.println("Requesting firmware info after connection...");
            requestFirmwareAttributes();
            
            return true;
        } else {
            Serial.printf("Failed, rc=%d\n", mqttClient.state());
            return false;
        }
    }
    return true;
}

// Yêu cầu thông tin về firmware
void requestFirmwareAttributes() {
    static int requestId = 0;
    requestId++;
    
    // Yêu cầu các thuộc tính chia sẻ liên quan đến firmware
    String requestTopic = "v1/devices/me/attributes/request/" + String(requestId);
    String requestPayload = "{\"sharedKeys\":\"fw_checksum,fw_checksum_algorithm,fw_size,fw_title,fw_version,fw_chunk_size\"}";
    
    Serial.printf("Requesting firmware attributes (ID: %d)...\n", requestId);
    Serial.printf("Topic: %s\n", requestTopic.c_str());
    Serial.printf("Payload: %s\n", requestPayload.c_str());
    
    bool published = mqttClient.publish(requestTopic.c_str(), requestPayload.c_str());
    Serial.printf("Publish result: %s\n", published ? "Success" : "Failed");
}

// Task OTA
void otaTask(void *pvParameters) {
    bool updateInitialized = false;
    unsigned long lastCheckTime = 0;
    unsigned long lastConnectionCheckTime = 0;
    const unsigned long CHECK_INTERVAL = 100000; // Kiểm tra firmware mới mỗi 100 giây
    const unsigned long CONNECTION_CHECK_INTERVAL = 10000; // Kiểm tra kết nối mỗi 10 giây
    
    Serial.println("OTA Task started");
    
    // Khởi tạo thiết bị
    initializeDevice();
    
    for (;;) {
        // Xử lý các thông điệp MQTT đến cho OTA
        mqttClient.loop();
        
        // Kiểm tra kết nối MQTT định kỳ
        if (millis() - lastConnectionCheckTime > CONNECTION_CHECK_INTERVAL) {
            if (!mqttClient.connected()) {
                Serial.println("MQTT disconnected, attempting to reconnect...");
                connectDeviceToThingsBoard();
            } else {
                Serial.printf("MQTT State: %d\n", mqttClient.state());
            }
            lastConnectionCheckTime = millis();
        }
        
        // Kiểm tra firmware mới định kỳ
        if (millis() - lastCheckTime > CHECK_INTERVAL) {
            if (deviceConnected) {
                Serial.println("Periodic firmware check...");
                requestFirmwareAttributes();
                lastCheckTime = millis();
            }
        }
        
        if (otaInProgress && !waitingForChunk && offset < fw_size) {      
            if (!updateInitialized) {
                Serial.println("Initializing update process...");
                if (!Update.begin(fw_size, U_FLASH)) {
                    Serial.println("Failed to begin OTA: " + String(Update.errorString()));
                    mqttClient.publish("v1/devices/me/attributes", 
                        "{\"fw_state\":\"FAILED\",\"fw_error\":\"Failed to initialize update\"}");
                    otaInProgress = false;
                    continue;
                }
                updateInitialized = true;
                Serial.println("OTA update initialized successfully");
                mqttClient.publish("v1/devices/me/attributes", "{\"fw_state\":\"DOWNLOADING\"}");
            }

            int chunkIndex = chunks_received;
            String payload = String(chunk_size);
            
            String reqTopic = "v2/fw/request/" + String(currentFirmwareRequestId) + "/chunk/" + String(chunkIndex);
            Serial.printf("Requesting chunk %d (offset: %d) with topic: %s\n", chunkIndex, offset, reqTopic.c_str());
            
            mqttClient.publish(reqTopic.c_str(), payload.c_str(), 1);
            
            waitingForChunk = true;
            lastRequestTime = millis();
            Serial.printf("Requested chunk %d at offset %d\n", chunkIndex, offset);
        }
        
        if (waitingForChunk && millis() - lastRequestTime > OTA_REQUEST_TIMEOUT) {
            Serial.println("Timeout waiting for chunk. Retrying...");
            waitingForChunk = false;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}