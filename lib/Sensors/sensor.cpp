#include <sensor.hpp>

DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135_sensor(MQ135_PIN); 

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

//hàm đánh giá chất lượng không khí
String getAQICategory(int aqi) {
  if (aqi <= 50) {
    return "Good";
  } else if (aqi <= 100) {
    return "Moderate";
  } else if (aqi <= 200) {
    return "Unhealthy";
  } else if (aqi <= 300) {
    return "Very Unhealthy";
  } else {
    return "Hazardous";
  }
}

void readMQ135(void *pvParameters) {
  while (1) {
    int rawValue = analogRead(MQ135_PIN);  // Đọc từ cảm biến MQ135
    int mappedValue = map(rawValue, 0, 4096, 0, 1024);  // scale về 0-1024
    airQuality = mappedValue;

    category = getAQICategory(airQuality);  // Đánh giá chất lượng

    Serial.printf("Chất lượng không khí (MQ135): %d (%s)\n", airQuality, category.c_str());

    if (tb.connected()) {
      tb.sendTelemetryData("air_quality", airQuality);         // Giá trị thô
      tb.sendTelemetryData("air_quality_category", category.c_str());  // Nhãn mô tả
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Gửi mỗi 5 giây
  }
}


