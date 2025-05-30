#include <sensor.hpp>
#include <global.hpp>
DHT dht(DHTPIN, DHTTYPE);
DHT20 dht20;
MQ135 mq135_sensor(MQ135_PIN);

UltraSonicDistanceSensor* ultrasonicSensor[10];  // hoặc struct wrapper
const char* SLOT_NAMES[10] = {
  "slot_A1", "slot_A2", "slot_A3", "slot_A4", "slot_A5",
  "slot_A6", "slot_A7", "slot_A8", "slot_A9", "slot_A10"
};
bool CarDetected[10]= {false};

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

// hàm đánh giá chất lượng không khí
String getAQICategory(int aqi)
{
  if (aqi <= 50)
  {
    return "Good";
  }
  else if (aqi <= 100)
  {
    return "Moderate";
  }
  else if (aqi <= 200)
  {
    return "Unhealthy";
  }
  else if (aqi <= 300)
  {
    return "Very Unhealthy";
  }
  else
  {
    return "Hazardous";
  }
}

void readMQ135(void *pvParameters)
{
  while (1)
  {
    int rawValue = analogRead(MQ135_PIN);              // Đọc từ cảm biến MQ135
    int mappedValue = map(rawValue, 0, 4096, 0, 1024); // scale về 0-1024
    airQuality = mappedValue;

    category = getAQICategory(airQuality); // Đánh giá chất lượng

    Serial.printf("Chất lượng không khí (MQ135): %d (%s)\n", airQuality, category.c_str());

    if (tb.connected())
    {
      tb.sendTelemetryData("air_quality", airQuality);                // Giá trị thô
      tb.sendTelemetryData("air_quality_category", category.c_str()); // Nhãn mô tả
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Gửi mỗi 5 giây
  }
}

void ultrasonicTask(void *pvParameters)
{
  int lastPirState = LOW; // Lưu trạng thái trước đó

  while (1)
  {
    int pirState = digitalRead(pirPin);

    // Phát hiện cạnh lên: LOW -> HIGH
    if (pirState == HIGH && lastPirState == LOW)
    {
      peopleCount++;
      Serial.printf("Số người hiện tại: %d\n", peopleCount);
    }
    else if (pirState == LOW && lastPirState == HIGH)
    {
      Serial.println("Không có người");
    }

    lastPirState = pirState; // Cập nhật trạng thái trước đó
    vTaskDelay(pdMS_TO_TICKS(10)); // Giảm thời gian kiểm tra để bắt cạnh chính xác hơn
  }
}

void carslotTask(void *pvParameters) {
  const float DETECTION_THRESHOLD = 10.0;

  vTaskDelay(pdMS_TO_TICKS(5000)); // Delay ban đầu

  for (;;) {
    for (int i = 0; i < 10; i++) {
      if (ultrasonicSensor[i] == NULL) continue;

      float distance = ultrasonicSensor[i]->measureDistanceCm();
      if (distance >= 0) {
        bool currentState = (distance < DETECTION_THRESHOLD);
        Serial.printf("[Slot %s] Distance: %.2f cm, Occupied: %s\n",
                      SLOT_NAMES[i], distance, currentState ? "true" : "false");

        if (currentState != CarDetected[i] && tb.connected()) {
          tb.sendTelemetryData(SLOT_NAMES[i], currentState ? "true" : "false");
          CarDetected[i] = currentState;
          Serial.printf("→ Sent telemetry: %s = %s\n", SLOT_NAMES[i], currentState ? "occupied" : "free");
        }
      } else {
        Serial.printf("[Slot %s] Sensor error!\n", SLOT_NAMES[i]);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000)); // Chờ rồi quét tiếp
  }
}



void pirTask(void *pvParameters) {
    // Khởi tạo cảm biến PIR
    pinMode(PIR_PIN2, INPUT);
    bool previousMotionState = false;
    unsigned long lastDetectionTime = 0;
    const unsigned long MOTION_TIMEOUT = 30000; // 30 giây timeout cho trạng thái chuyển động
    
    // Chờ cảm biến PIR ổn định (thường cần khoảng 60 giây)
    Serial.println("Khởi tạo cảm biến PIR...");
    vTaskDelay(pdMS_TO_TICKS(10000)); // Đợi 10 giây
    Serial.println("Cảm biến PIR đã sẵn sàng");
    
    for (;;) {
        // Đọc trạng thái cảm biến PIR
        bool currentMotionState = digitalRead(PIR_PIN2);
        unsigned long currentTime = millis();
        
        // Phát hiện chuyển động
        if (currentMotionState == HIGH) {
            lastDetectionTime = currentTime;
            
            // Nếu trạng thái thay đổi từ không có chuyển động sang có chuyển động
            if (!previousMotionState) {
                Serial.println("Phát hiện chuyển động!");
                motionDetected = true;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendTelemetryData("motion", "true");
                    Serial.println("Đã gửi thông báo chuyển động lên ThingsBoard");
                }
            }
            previousMotionState = true;
        } 
        // Không có chuyển động hoặc hết thời gian timeout
        else if (currentMotionState == LOW || (currentTime - lastDetectionTime > MOTION_TIMEOUT)) {
            // Nếu trạng thái thay đổi từ có chuyển động sang không có chuyển động
            if (previousMotionState) {
                Serial.println("Không phát hiện chuyển động");
                motionDetected = false;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendTelemetryData("motion", "false");
                    Serial.println("Đã gửi thông báo không có chuyển động lên ThingsBoard");
                }
            }
            previousMotionState = false;        }
        
        // Cập nhật mỗi 5 giây theo yêu cầu
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
