#include <sensor.hpp>

DHT20 dht20;
int airQuality = 0;
MQ135 mq135_sensor(MQ135_PIN); 

void readDHT20(void *pvParameters)
{
  unsigned long lastReadTime = 0; // Lưu thời gian đọc gần nhất
  const unsigned long READ_INTERVAL = 15000; // 15 giây đọc một lần

  while (1)
  {
    if (millis() - lastReadTime >= READ_INTERVAL)
    {                          // Kiểm tra nếu đã qua 15 giây
      lastReadTime = millis(); // Cập nhật thời gian đọc mới nhất

      dht20.read();
      float temp = dht20.getTemperature();
      float hum = dht20.getHumidity();

      if (!isnan(temp) && !isnan(hum))
      {
        if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY))
        {
          temperature = temp;
          humidity = hum;
          xSemaphoreGive(sensorDataMutex);
        }
        Serial.printf("Nhiệt độ: %.2f °C | Độ ẩm: %.2f %%\n", temp, hum);
      }      else
      {
        Serial.println("Lỗi! Không thể đọc từ DHT20.");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Kiểm tra mỗi 1 giây
  }
}

void readMQ135(void *pvParameters) {
  while (1) {
    // int rawValue = analogRead(MQ135_PIN); // Đọc giá trị analog từ MQ135
    // airQuality = rawValue;

    float ppm = mq135_sensor.getPPM();
    float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);

    Serial.printf("Chất lượng không khí (MQ135): %d\n", correctedPPM);    if (tb.connected()) {
      tb.sendTelemetryData("air_quality", correctedPPM); // Gửi lên CoreIoT
    }

    vTaskDelay(pdMS_TO_TICKS(15000)); // Gửi mỗi 15 giây
  }
}