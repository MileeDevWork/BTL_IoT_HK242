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

//hàm đánh giá mật độ dân số khu vực
String getDensityLevel(float density) {
  if (density <= 0.2)
    return "Good";
  else if (density <= 0.5)
    return "Warning";
  else
    return "Overload";
}
void peopleCountingTask(void *pvParameters)
{
  const DeviceConfig* config = getCurrentConfig();
  
  if (!config->enablePIR) {
    Serial.println("PIR sensor disabled for this device type");
    vTaskDelete(NULL);
    return;
  }

  int lastPirInState = LOW;   // Trạng thái trước đó của cảm biến vào
  int lastPirOutState = LOW;  // Trạng thái trước đó của cảm biến ra

  unsigned long lastSendTime = 0; // Lưu thời gian lần gửi mật độ gần nhất
  
  // Get dynamic PIR pins
  int pirPinIn = getPIRPin();
  int pirPinOut = getPIRPin2();
  
  if (pirPinIn < 0 || pirPinOut < 0) {
    Serial.println("ERROR: Invalid PIR pin configuration");
    vTaskDelete(NULL);
    return;
  }
  
  pinMode(pirPinIn, INPUT);
  pinMode(pirPinOut, INPUT);
  
  Serial.printf("PIR sensors initialized on pins IN=%d, OUT=%d for %s\n", pirPinIn, pirPinOut, config->deviceType);

  while (1)
  {
    int pirInState = digitalRead(pirPinIn);
    int pirOutState = digitalRead(pirPinOut);

    // Đếm người vào (cạnh lên cảm biến vào)
    if (pirInState == HIGH && lastPirInState == LOW)
    {
      peopleCount++;
      Serial.printf("[%s] Số người hiện tại: %d\n", config->deviceType, peopleCount);
    }

    // Đếm người ra (cạnh lên cảm biến ra)
    if (pirOutState == HIGH && lastPirOutState == LOW)
    {
      if (peopleCount > 0) peopleCount--;
      Serial.printf("Số người hiện tại: %d\n", peopleCount);
      Serial.printf("[%s] Không có người\n", config->deviceType);
    }

    // Cập nhật lại trạng thái cũ
    lastPirInState = pirInState;
    lastPirOutState = pirOutState;

    // Tính mật độ và gửi lên ThingsBoard mỗi 10 giây
    if (millis() - lastSendTime >= 10000)
    {
      lastSendTime = millis();

      float density = peopleCount / AREA_SQUARE_METERS;
      String densityLevel = getDensityLevel(density);

      Serial.printf("Mật độ dân số: %.2f người/m² (%s)\n", density, densityLevel.c_str());

      if (tb.connected())
      {
        // tb.sendTelemetryData("density", density);
        tb.sendTelemetryData("densityLevel", densityLevel.c_str());
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Hàm khởi tạo thống kê bãi đỗ xe
void initParkingStats() {
    totalParkingSlots = 1;  // Chỉ 1 slot A1 cho mô phỏng
    occupiedSlots = 0;
    availableSlots = totalParkingSlots;
    
    Serial.printf("Parking stats initialized: %d total slots\n", totalParkingSlots);
}

// Nhiệm vụ quản lý vị trí đỗ xe qua cảm biến siêu âm (Mô phỏng với 1 slot duy nhất)
void carslotTask(void *pvParameters) {
  const DeviceConfig* config = getCurrentConfig();
  
  // Skip if not enabled
  if (!config->hasUltrasonic) {
    Serial.println("Ultrasonic sensors disabled for this device type");
    vTaskDelete(NULL);
    return;
  }
  
  initUltrasonicSensors();
  initParkingStats();
  unsigned long lastStatsUpdate = 0;
  
  vTaskDelay(pdMS_TO_TICKS(PARKING_INITIAL_DELAY)); // Delay ban đầu

  for (;;) {
    bool parkingStateChanged = false;
    
    // Chỉ xử lý slot A1 (index 0)
    int slotIndex = 0; // slot_A1
    
    if (ultrasonicSensor[slotIndex] != NULL) {      float distance = ultrasonicSensor[slotIndex]->measureDistanceCm();
      if (distance >= 0) {
        bool currentState = (distance < PARKING_DETECTION_THRESHOLD);
        Serial.printf("[Slot %s] Distance: %.2f cm, Occupied: %s\n",
                      SLOT_NAMES[slotIndex], distance, currentState ? "true" : "false");
        
        // Kiểm tra thay đổi trạng thái và gửi telemetry cho slot A1
        if (currentState != CarDetected[slotIndex] && tb.connected()) {
          tb.sendTelemetryData(SLOT_NAMES[slotIndex], currentState ? "true" : "false");
          CarDetected[slotIndex] = currentState;
          parkingStateChanged = true;
          Serial.printf("→ Sent telemetry: %s = %s\n", SLOT_NAMES[slotIndex], currentState ? "occupied" : "free");
        }
      } else {
        Serial.printf("[Slot %s] Sensor error!\n", SLOT_NAMES[slotIndex]);
      }
    }
    
    // Cập nhật thống kê bãi đỗ xe nếu có thay đổi hoặc đã đến thời gian cập nhật
    if (parkingStateChanged || (millis() - lastStatsUpdate >= PARKING_STATS_UPDATE_INTERVAL)) {
      updateParkingStats();
      sendParkingDataToThingsBoard();
      lastStatsUpdate = millis();
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

// Initialize ultrasonic sensors with dynamic pins (Chỉ khởi tạo cho slot A1)
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
    
    Serial.printf("Initializing 1 ultrasonic sensor for slot A1 on pins TRIG=%d, ECHO=%d\n", 
                  trigPin, echoPin);
    
    // Chỉ khởi tạo sensor cho slot A1 (index 0)
    ultrasonicSensor[0] = new UltraSonicDistanceSensor(trigPin, echoPin);
    Serial.printf("Slot %s sensor initialized\n", SLOT_NAMES[0]);
    
    // Đảm bảo các slot khác là NULL
    for (int i = 1; i < 10; i++) {
        ultrasonicSensor[i] = nullptr;
    }
}

// Hàm cập nhật thống kê bãi đỗ xe (Chỉ tính cho slot A1)
void updateParkingStats() {
    const DeviceConfig* config = getCurrentConfig();
    
    if (!config->hasUltrasonic) {
        return;
    }
    
    // Chỉ kiểm tra slot A1 (index 0)
    occupiedSlots = CarDetected[0] ? 1 : 0;
    
    // Cập nhật số slot còn trống (chỉ có 1 slot)
    availableSlots = totalParkingSlots - occupiedSlots;
    
    // Tính phần trăm lấp đầy
    float occupancyRate = (totalParkingSlots > 0) ? ((float)occupiedSlots / totalParkingSlots) * 100.0 : 0.0;
    
    Serial.printf("[Parking Stats] Total: %d | Occupied: %d | Available: %d | Occupancy: %.1f%% (Slot A1 only)\n", 
                  totalParkingSlots, occupiedSlots, availableSlots, occupancyRate);
}

// Hàm gửi dữ liệu thống kê bãi đỗ xe lên ThingsBoard
void sendParkingDataToThingsBoard() {
    const DeviceConfig* config = getCurrentConfig();
    
    if (!config->hasUltrasonic || !tb.connected()) {
        return;
    }
    
    // Tính phần trăm lấp đầy
    float occupancyRate = (totalParkingSlots > 0) ? ((float)occupiedSlots / totalParkingSlots) * 100.0 : 0.0;
      // Xác định trạng thái bãi đỗ xe (cho 1 slot)
    String parkingStatus;
    if (occupancyRate >= 95.0) {
        parkingStatus = "Full";
    } else if (occupancyRate >= 80.0) {
        parkingStatus = "Nearly Full";
    } else if (occupancyRate >= 50.0) {
        parkingStatus = "Half Full";
    } else if (occupancyRate >= 20.0) {
        parkingStatus = "Available";
    } else {
        parkingStatus = "Mostly Empty";
    }
    
    // Gửi dữ liệu thống kê lên ThingsBoard
    tb.sendTelemetryData("total_parking_slots", totalParkingSlots);
    tb.sendTelemetryData("occupied_slots", occupiedSlots);
    tb.sendTelemetryData("available_slots", availableSlots);
    tb.sendTelemetryData("occupancy_rate", occupancyRate);
    tb.sendTelemetryData("parking_status", parkingStatus.c_str());
    
    Serial.printf("→ Sent parking stats to ThingsBoard: %d available, %.1f%% occupied (%s) - Slot A1 only\n", 
                  availableSlots, occupancyRate, parkingStatus.c_str());
}
