#include <mqtt.hpp>

// Variable definitions for extern declarations in mqtt.hpp
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient);

// Nhiệm vụ kết nối và gửi dữ liệu lên ThingsBoard
void TaskThingsBoard(void *pvParameters)
{
  const DeviceConfig* config = getCurrentConfig();

  while (1)
  {
    if (!reconnect())
    {
      vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi trước khi thử lại
      continue;
    }

    if (!tb.connected())
    {
      Serial.printf("Đang kết nối ThingsBoard %s...\n", config->deviceType);
      if (!tb.connect(THINGSBOARD_SERVER, config->token, THINGSBOARD_PORT))
      {
        Serial.printf("Kết nối %s thất bại!\n", config->deviceType);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Thử lại sau 5 giây
        continue;
      }
      tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
      tb.sendAttributeData("deviceType", config->deviceType);
      tb.sendAttributeData("deviceName", config->deviceName);
      Serial.printf("Kết nối ThingsBoard %s thành công!\n", config->deviceType);
    }

    // Gửi dữ liệu môi trường nếu được bật
    if (config->enableTempHumidity && (millis() - previousDataSend > config->envSensorInterval))
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
        Serial.printf("Gửi dữ liệu môi trường lên ThingsBoard %s...\n", config->deviceType);
        tb.sendTelemetryData("temperature", temp);
        tb.sendTelemetryData("humidity", hum);
      }
      else
      {
        Serial.println("Không có dữ liệu môi trường hợp lệ để gửi!");
      }
    }

    tb.loop();                       // Xử lý MQTT
    vTaskDelay(pdMS_TO_TICKS(1000)); // Kiểm tra mỗi giây
  }
}
