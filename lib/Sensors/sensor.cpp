#include <sensor.hpp>
#include <global.hpp>
#include <config.hpp>

// Global sensor objects - will be initialized with dynamic pins
DHT* dht = nullptr;
DHT20 dht20;
MQ135* mq135_sensor = nullptr;

UltraSonicDistanceSensor* ultrasonicSensor[10];  // hoặc struct wrapper
const char* SLOT_NAMES[10] = {
  "slot_A1", "slot_A2", "slot_A3", "slot_A4", "slot_A5",
  "slot_A6", "slot_A7", "slot_A8", "slot_A9", "slot_A10"
};
bool CarDetected[10]= {false};

// Hàm đọc dữ liệu từ cảm biến DHT11 (nhiệt độ và độ ẩm)
void readDHT11(void *pvParameters)
{
  const DeviceConfig* config = getCurrentConfig();
  
  if (!config->enableTempHumidity) {
    Serial.println("DHT11 disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  // Initialize DHT sensor with dynamic pin
  if (dht == nullptr) {
    dht = new DHT(getDHTPin(), DHTTYPE);
    dht->begin();
    Serial.printf("DHT11 initialized on pin %d for %s\n", getDHTPin(), config->deviceType);
  }

  unsigned long lastReadTime = 0;

  while (1)
  {
    if (millis() - lastReadTime >= 2000)
    {
      lastReadTime = millis();

      float temp = dht->readTemperature();
      float hum = dht->readHumidity();

      if (!isnan(temp) && !isnan(hum))
      {
        if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY))
        {
          temperature = temp;
          humidity = hum;
          xSemaphoreGive(sensorDataMutex);
        }
        Serial.printf("[%s] Nhiệt độ: %.2f °C | Độ ẩm: %.2f %%\n", config->deviceType, temp, hum);
      }
      else
      {
        Serial.println("Lỗi! Không thể đọc từ DHT11.");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Hàm đọc dữ liệu từ cảm biến DHT20 (nhiệt độ và độ ẩm)
void readDHT20(void *pvParameters)
{
  unsigned long lastReadTime = 0;  const DeviceConfig* config = getCurrentConfig();
  
  if (!config->enableTempHumidity) {
    Serial.println("DHT20 disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  while (1)
  {
    if (millis() - lastReadTime >= config->envSensorInterval)
    {
      lastReadTime = millis();

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
        Serial.printf("[%s] Nhiệt độ: %.2f °C | Độ ẩm: %.2f %%\n", config->deviceType, temp, hum);
      }
      else
      {
        Serial.println("Lỗi! Không thể đọc từ DHT20.");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Xác định hạng chất lượng không khí dựa trên chỉ số AQI
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

// Đọc và gửi dữ liệu chất lượng không khí từ MQ135
void readMQ135(void *pvParameters)
{
  const DeviceConfig* config = getCurrentConfig();
  
  if (!config->enableAirQuality) {
    Serial.println("MQ135 disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  // Initialize MQ135 sensor with dynamic pin
  if (mq135_sensor == nullptr) {
    mq135_sensor = new MQ135(getMQ135Pin());
    Serial.printf("MQ135 initialized on pin %d for %s\n", getMQ135Pin(), config->deviceType);
  }

  while (1)
  {
    int rawValue = analogRead(getMQ135Pin());
    int mappedValue = map(rawValue, 0, 4096, 0, 1024);
    airQuality = mappedValue;

    category = getAQICategory(airQuality);

    Serial.printf("[%s] Chất lượng không khí (MQ135): %d (%s)\n", config->deviceType, airQuality, category.c_str());

    if (tb.connected())
    {
      tb.sendTelemetryData("air_quality", airQuality);
      tb.sendTelemetryData("air_quality_category", category.c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// Nhiệm vụ đếm số người dựa trên cảm biến PIR
void ultrasonicTask(void *pvParameters)
{
  const DeviceConfig* config = getCurrentConfig();
  
  if (!config->enablePIR) {
    Serial.println("PIR sensor disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  int lastPirState = LOW; // Lưu trạng thái trước đó
  int pirPin = getPIRPin(); // Get dynamic PIR pin
  pinMode(pirPin, INPUT);
  
  Serial.printf("PIR initialized on pin %d for %s\n", pirPin, config->deviceType);

  while (1)
  {
    int pirState = digitalRead(pirPin);

    // Phát hiện cạnh lên: LOW -> HIGH
    if (pirState == HIGH && lastPirState == LOW)
    {
      peopleCount++;
      Serial.printf("[%s] Số người hiện tại: %d\n", config->deviceType, peopleCount);
    }
    else if (pirState == LOW && lastPirState == HIGH)
    {
      Serial.printf("[%s] Không có người\n", config->deviceType);
    }

    lastPirState = pirState; // Cập nhật trạng thái trước đó
    vTaskDelay(pdMS_TO_TICKS(10)); // Giảm thời gian kiểm tra để bắt cạnh chính xác hơn
  }
}

// Nhiệm vụ quản lý vị trí đỗ xe qua cảm biến siêu âm
void carslotTask(void *pvParameters) {
  const DeviceConfig* config = getCurrentConfig();
  
  // Skip if not enabled
  if (!config->hasUltrasonic) {
    Serial.println("Ultrasonic sensors disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  // Initialize ultrasonic sensors with dynamic pins
  initUltrasonicSensors();

  const float DETECTION_THRESHOLD = 10.0;
  vTaskDelay(pdMS_TO_TICKS(5000)); // Delay ban đầu

  for (;;) {
    for (int i = 0; i < config->ultrasonicSlots; i++) {
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
    vTaskDelay(pdMS_TO_TICKS(config->ultrasonicInterval));
  }
}

// Nhiệm vụ theo dõi chuyển động từ cảm biến PIR
void pirTask(void *pvParameters) {
    const DeviceConfig* config = getCurrentConfig();
    
    // Skip if not enabled
    if (!config->enablePIR) {
        Serial.println("PIR sensor disabled for this device type");
        vTaskDelete(NULL);
        return;
    }
    
    // Khởi tạo cảm biến PIR với pin động
    int pir2Pin = getPIRPin2(); // Get dynamic PIR2 pin
    pinMode(pir2Pin, INPUT);
    bool previousMotionState = false;
    unsigned long lastDetectionTime = 0;
    const unsigned long MOTION_TIMEOUT = 30000; // 30 giây timeout cho trạng thái chuyển động
    
    // Chờ cảm biến PIR ổn định (thường cần khoảng 60 giây)
    Serial.printf("Khởi tạo cảm biến PIR trên pin %d cho %s...\n", pir2Pin, config->deviceType);
    vTaskDelay(pdMS_TO_TICKS(10000)); // Đợi 10 giây
    Serial.printf("Cảm biến PIR cho %s đã sẵn sàng\n", config->deviceType);
    
    for (;;) {
        // Đọc trạng thái cảm biến PIR
        bool currentMotionState = digitalRead(pir2Pin);
        unsigned long currentTime = millis();
        
        // Phát hiện chuyển động
        if (currentMotionState == HIGH) {
            lastDetectionTime = currentTime;
            
            // Nếu trạng thái thay đổi từ không có chuyển động sang có chuyển động
            if (!previousMotionState) {
                if (strcmp(config->deviceType, DEVICE_TYPE_BUILDING) == 0) {
                    // Building mode: People counting
                    peopleCount++;
                    Serial.printf("[%s] Phát hiện người! Số người hiện tại: %d\n", config->deviceType, peopleCount);
                } else if (strcmp(config->deviceType, DEVICE_TYPE_CARPARK) == 0) {
                    // Carpark mode: Security detection
                    Serial.printf("[%s] Phát hiện chuyển động! (Bảo mật)\n", config->deviceType);
                }
                
                motionDetected = true;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    if (strcmp(config->deviceType, DEVICE_TYPE_BUILDING) == 0) {
                        tb.sendTelemetryData("people_count", peopleCount);
                        tb.sendTelemetryData("motion", "true");
                    } else {
                        tb.sendTelemetryData("motion", "true");
                    }
                    Serial.printf("Đã gửi thông báo chuyển động lên ThingsBoard %s\n", config->deviceType);
                }
            }
            previousMotionState = true;
        } 
        // Không có chuyển động hoặc hết thời gian timeout
        else if (currentMotionState == LOW || (currentTime - lastDetectionTime > MOTION_TIMEOUT)) {
            // Nếu trạng thái thay đổi từ có chuyển động sang không có chuyển động
            if (previousMotionState) {
                Serial.printf("[%s] Không phát hiện chuyển động\n", config->deviceType);
                motionDetected = false;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendTelemetryData("motion", "false");
                    Serial.printf("Đã gửi thông báo không có chuyển động lên ThingsBoard %s\n", config->deviceType);
                }
            }
            previousMotionState = false;
        }
        
        // Cập nhật theo config interval
        vTaskDelay(pdMS_TO_TICKS(config->pirInterval));
    }
}

// Nhiệm vụ đọc RFID (thô)
void rfidTask(void *pvParameters) {
  const DeviceConfig* config = getCurrentConfig();
  
  // Skip if not enabled
  if (!config->hasRFID) {
    Serial.println("RFID disabled for this device type");
    vTaskDelete(NULL);
    return;
  }
  
  Serial.printf("RFID task started for %s\n", config->deviceType);
  
  while (1) {
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
  }
}

// Initialize ultrasonic sensors with dynamic pins
void initUltrasonicSensors() {
    const DeviceConfig* config = getCurrentConfig();
    
    if (!config->hasUltrasonic) {
        return;
    }
    
    int trigPin = getUltrasonicTrigPin();
    int echoPin = getUltrasonicEchoPin();
    
    if (trigPin < 0 || echoPin < 0) {
        Serial.println("ERROR: Invalid ultrasonic pin configuration");
        return;
    }
    
    Serial.printf("Initializing %d ultrasonic sensors on pins TRIG=%d, ECHO=%d\n", 
                  config->ultrasonicSlots, trigPin, echoPin);
    
    for (int i = 0; i < config->ultrasonicSlots; i++) {
        // For simplicity, using the same TRIG/ECHO pins for all slots
        // In a real implementation, you might need multiplexing or different pins
        ultrasonicSensor[i] = new UltraSonicDistanceSensor(trigPin, echoPin);
        Serial.printf("Slot %s sensor initialized\n", SLOT_NAMES[i]);
    }
}
