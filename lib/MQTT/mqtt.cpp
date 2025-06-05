#include <mqtt.hpp>
#include <wifi.hpp> // Thêm dòng này để định nghĩa WIFI_SSID và WIFI_PASSWORD

// Variable definitions for extern declarations in mqtt.hpp
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// for scheduler task
QueueHandle_t ledStateQueue; // Hàng đợi lưu trạng thái LED
const char* ledStateControlKey = "ledState"; // Key của shared attribute

// Hàm kiểm tra và kết nối lại WiFi
// bool reconnect() {
//     if (WiFi.status() != WL_CONNECTED) {
//         Serial.print("Connecting to WiFi...");
//         WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // WIFI_SSID và WIFI_PASSWORD được định nghĩa trong wifi.hpp hoặc config.hpp
//         int attempts = 0;
//         while (WiFi.status() != WL_CONNECTED && attempts < 10) {
//             delay(500);
//             Serial.print(".");
//             attempts++;
//         }
//         if (WiFi.status() != WL_CONNECTED) {
//             Serial.println("Failed to connect to WiFi");
//             return false;
//         }
//         Serial.println("Connected to WiFi");
//     }
//     return true;
// }

// Hàm điều khiển LED
void ledControlTask(void *pvParameters) {
    bool currentLedState = false;
    pinMode(LED_PIN, OUTPUT); // Đảm bảo LED_PIN được cấu hình
    digitalWrite(LED_PIN, LOW); // Đặt trạng thái ban đầu
    Serial.println("LED Control Task started");
    for (;;) {
        if (xQueueReceive(ledStateQueue, &currentLedState, portMAX_DELAY) == pdTRUE) {
            digitalWrite(LED_PIN, currentLedState ? HIGH : LOW);
            Serial.printf("Setting LED to: %s\n", currentLedState ? "ON" : "OFF");
        } else {
            Serial.println("Failed to receive LED state from queue");
        }
    }
}

// Callback xử lý tin nhắn MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Tin nhắn đến từ topic: ");
    Serial.println(topic);
    Serial.print("Nội dung: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strstr(topic, "v1/devices/me/attributes")) {
        Serial.println("Xử lý thuộc tính chia sẻ...");
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
            Serial.printf("Lỗi parse JSON: %s\n", error.c_str());
            return;
        }

        if (doc.containsKey("ledState")) {
            String ledStateStr = doc["ledState"].as<String>();
            Serial.print("Giá trị ledState: ");
            Serial.println(ledStateStr);

            bool newLedState = (ledStateStr == "ON");
            if (xQueueSend(ledStateQueue, &newLedState, 0) != pdTRUE) {
                Serial.println("Gửi trạng thái LED vào hàng đợi thất bại!");
            } else {
                Serial.print("Đã gửi ledState đến task điều khiển LED: ");
                Serial.println(newLedState ? "ON" : "OFF");
            }
        } else {
            Serial.println("Không tìm thấy thuộc tính ledState!");
        }
    }
}

// Task kết nối và gửi dữ liệu lên ThingsBoard
void TaskThingsBoard(void *pvParameters) {
    const DeviceConfig* config = getCurrentConfig();

    mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
    mqttClient.setCallback(mqttCallback);

    while (1) {
        if (!reconnect()) {
            vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi trước khi thử lại
            continue;
        }

        if (!mqttClient.connected()) {
            Serial.printf("Đang kết nối ThingsBoard %s...\n", config->deviceType);
            if (!mqttClient.connect("ESP32Client", config->token, nullptr)) {
                Serial.printf("Kết nối %s thất bại, rc=%d\n", config->deviceType, mqttClient.state());
                vTaskDelay(pdMS_TO_TICKS(5000)); // Thử lại sau 5 giây
                continue;
            }
            Serial.printf("Kết nối ThingsBoard %s thành công!\n", config->deviceType);

            // Gửi thông tin thiết bị
            String macAddress = WiFi.macAddress();
            String deviceType = String(config->deviceType);
            String deviceName = String(config->deviceName);
            String attributePayload = "{\"macAddress\":\"" + macAddress + "\",\"deviceType\":\"" + deviceType + "\",\"deviceName\":\"" + deviceName + "\"}";
            mqttClient.publish("v1/devices/me/attributes", attributePayload.c_str());

            // Đăng ký topic để nhận shared attributes
            mqttClient.subscribe("v1/devices/me/attributes");

            // Yêu cầu giá trị ban đầu của ledState
            String payload = "{\"shared\":[\"" + String(ledStateControlKey) + "\"]}";
            mqttClient.publish("v1/devices/me/attributes", payload.c_str());
            Serial.println("Yêu cầu giá trị ledState từ ThingsBoard");
        }

        // Gửi dữ liệu môi trường nếu được bật
        if (config->enableTempHumidity && (millis() - previousDataSend > config->envSensorInterval)) {
            previousDataSend = millis();

            float temp, hum;
            if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY)) {
                temp = temperature;
                hum = humidity;
                xSemaphoreGive(sensorDataMutex);
            }

            if (!isnan(temp) && !isnan(hum)) {
                Serial.printf("Gửi dữ liệu môi trường lên ThingsBoard %s...\n", config->deviceType);
                String telemetryPayload = "{\"temperature\":" + String(temp) + ",\"humidity\":" + String(hum) + "}";
                mqttClient.publish("v1/devices/me/telemetry", telemetryPayload.c_str());
            } else {
                Serial.println("Không có dữ liệu môi trường hợp lệ để gửi!");
            }
        }

        mqttClient.loop(); // Xử lý MQTT
        vTaskDelay(pdMS_TO_TICKS(1000)); // Kiểm tra mỗi giây
    }
}