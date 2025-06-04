#include <mqtt.hpp>

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient_internal(wifiClient);
Arduino_MQTT_Client &mqttClient = mqttClient_internal;  
ThingsBoard tb(mqttClient, 1024U); 

PubSubClient client(wifiClient);
const char* ledStateControlKey = "ledState";
volatile bool ledState = false;
QueueHandle_t ledStateQueue = NULL;

void TaskThingsBoard(void *pvParameters)
{
  uint32_t previousDataSend = 0;

  while (1)
  {
    if (!reconnect())
    {
      vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi trước khi thử lại
      continue;
    }

    if (!tb.connected())
    {
      Serial.println("Đang kết nối ThingsBoard...");
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
      {
        Serial.println("Kết nối thất bại!");
        vTaskDelay(pdMS_TO_TICKS(5000)); // Thử lại sau 5 giây
        continue;
      }
      tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
      Serial.println("Kết nối ThingsBoard thành công!");
    }

     // Đăng ký nhận thông tin shared attributes
    client.subscribe("v1/devices/me/attributes");
    // Yêu cầu giá trị ban đầu của các shared attributes
    String payload = "{\"shared\":[\"" + String(ledStateControlKey) + "\"]}";
    client.publish("v1/devices/me/attributes", payload.c_str());
    Serial.println("Sent request for shared attributes.");

    // Gửi dữ liệu mỗi 10 giây
    if (millis() - previousDataSend > telemetrySendInterval)
    {
      previousDataSend = millis();

      float temp, hum;
      if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY))
      {
        temp = temperature;
        hum = humidity;
        xSemaphoreGive(sensorDataMutex);
      }

      if (!isnan(temp) && !isnan(hum))
      {
        Serial.println("Gửi dữ liệu lên ThingsBoard...");
        tb.sendTelemetryData("temperature", temp);
        tb.sendTelemetryData("humidity", hum);
      }
      else
      {
        Serial.println("Không có dữ liệu hợp lệ để gửi!");
      }
    }

    tb.loop();                       // Xử lý MQTT
    vTaskDelay(pdMS_TO_TICKS(1000)); // Kiểm tra mỗi giây
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
    Serial.println("Callback function called in MQTT Task.");
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Xử lý phản hồi shared attributes
    if (strstr(topic, "attributes")) {
        Serial.println("Processing shared attributes response...");
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload, length);

        if (doc.containsKey("ledState")) {
            String ledStateStr = doc["ledState"].as<String>();
            Serial.print("ledState value from TB: ");
            Serial.println(ledStateStr);

            bool newLedState = false;
            if (ledStateStr == "ON") {
                newLedState = true;
                Serial.println("ledState is now TRUE");
            } else {
                newLedState = false;
                Serial.println("ledState is now FALSE");
            }

            // Send the new LED state to the LED control task via the queue
            if (xQueueSend(ledStateQueue, &newLedState, 0) != pdTRUE) {
                Serial.println("Failed to send LED state to queue.");
            }
            Serial.print("Sent ledState to LED Control Task: ");
            Serial.println(newLedState ? "ON" : "OFF");
        } else {
            Serial.println("Attribute 'ledState' not found in response.");
        }
    }
}

void ledControlTask(void *pvParameters) {
    bool currentLedState = false;
    for (;;) {
        if (xQueueReceive(ledStateQueue, &currentLedState, portMAX_DELAY) == pdTRUE) {
            digitalWrite(ledPin, currentLedState ? HIGH : LOW);
            Serial.print("Setting LED to: ");
            Serial.println(currentLedState ? "ON" : "OFF");
        }
        // No need for additional delay here as the task will wait for a message in the queue
    }
}