#include <sensor.hpp>

DHT dht(DHTPIN, DHTTYPE);

void readDHT11(void *pvParameters)
{
  unsigned long lastReadTime = 0; // Lưu thời gian đọc gần nhất

  while (1)
  {
    if (millis() - lastReadTime >= 2000)
    {                          // Kiểm tra nếu đã qua 2 giây
      lastReadTime = millis(); // Cập nhật thời gian đọc mới nhất

      float temp = dht.readTemperature();
      float hum = dht.readHumidity();

      if (!isnan(temp) && !isnan(hum))
      {
        if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY))
        {
          temperature = temp;
          humidity = hum;
          xSemaphoreGive(sensorDataMutex);
        }
        Serial.printf("Nhiệt độ: %.2f °C | Độ ẩm: %.2f %%\n", temp, hum);
      }
      else
      {
        Serial.println("Lỗi! Không thể đọc từ DHT11.");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // Giảm tải CPU, kiểm tra lại sau 500ms
  }
}