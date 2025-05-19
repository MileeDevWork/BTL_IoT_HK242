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
  // 1) Lấy R0 từ thư viện
  Serial.println("Khởi động và lấy R0 MQ135...");
  float r0 = mq135_sensor.getRZero();
  Serial.printf("→ R0 hiện tại của MQ135: %.2f\n", r0);

  // 2) Đợi warm-up
  vTaskDelay(pdMS_TO_TICKS(20000));
  Serial.println("MQ135 ready");

  // 3) Thiết lập buffer moving average
  const int MA_LEN = 5;
  float maBuf[MA_LEN] = {0};
  int   maIdx = 0;

  while (1) {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY)) {
      // Read raw and corrected PPM, then guard nan
      float rawPPM  = mq135_sensor.getPPM();
      float corrPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);
      if (isnan(rawPPM) || isnan(corrPPM)) {
        Serial.println("MQ135 reading invalid (nan), skipping");
        xSemaphoreGive(sensorDataMutex);
        vTaskDelay(pdMS_TO_TICKS(15000));
        continue;
      }
      xSemaphoreGive(sensorDataMutex);

      // 4) Kiểm tra và clamp
      if (corrPPM < 0 || corrPPM > 5000) {
        corrPPM = rawPPM;
      }
      corrPPM = max(corrPPM, 0.0f);

      // 5) Moving average
      maBuf[maIdx] = corrPPM;
      maIdx = (maIdx + 1) % MA_LEN;
      float sum = 0;
      for (int i = 0; i < MA_LEN; i++) sum += maBuf[i];
      float avgPPM = sum / MA_LEN;

      // 6) In và gửi
      Serial.printf("MQ135: raw=%.2f ppm, corr=%.2f ppm, avg=%.2f ppm\n", 
                     rawPPM, corrPPM, avgPPM);
      if (tb.connected()) {
        tb.sendTelemetryData("air_quality", avgPPM);
      }
    } else {
      Serial.println("Không lấy được mutex MQ135");
    }

    vTaskDelay(pdMS_TO_TICKS(15000)); // 15s/lần
  }
}