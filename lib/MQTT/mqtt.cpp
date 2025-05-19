#include <mqtt.hpp>
#include <OTA.h>

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient_internal(wifiClient);
Arduino_MQTT_Client &mqttClient = mqttClient_internal;  
ThingsBoard tb(mqttClient, 1024U); 

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

    // Kiểm tra kết nối ThingsBoard và OTA device
    if (!tb.connected() || (dhtDeviceIndex >= 0 && !devices[dhtDeviceIndex].connected))
    {
      // Kết nối TB chuẩn
      if (!tb.connected()) {
        Serial.println("Đang kết nối ThingsBoard...");
        if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
        {
          Serial.println("Kết nối thất bại!");
          vTaskDelay(pdMS_TO_TICKS(5000)); // Thử lại sau 5 giây
          continue;
        }
        
        // Gửi thuộc tính thiết bị
        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
        
        Serial.println("Kết nối ThingsBoard thành công!");
      }

      // Kết nối thiết bị OTA nếu chưa kết nối
      if (dhtDeviceIndex >= 0 && !devices[dhtDeviceIndex].connected) {
        connectDeviceToThingsBoard(dhtDeviceIndex);
      }
    }

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
      else      {
        Serial.println("Không có dữ liệu hợp lệ để gửi!");
      }
    }

    tb.loop();                       // Xử lý MQTT
    
    // Xử lý loop của thiết bị OTA nếu đã kết nối
    if (dhtDeviceIndex >= 0 && devices[dhtDeviceIndex].connected) {
      devices[dhtDeviceIndex].mqttClient->loop();
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Kiểm tra mỗi giây
  }
}
