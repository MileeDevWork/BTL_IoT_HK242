#include <Arduino.h>
#include <Wire.h>
#include <wifi.hpp>
#include <global.hpp>
#include <mqtt.hpp> 
#include <sensor.hpp>
#include "HCSR04.h"
#include "OTA.h"

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Khởi tạo các cảm biến
  dht20.begin();
  ultrasonicSensor = new UltraSonicDistanceSensor(SONIC_TRIGGER, SONIC_ECHO);
  
  // Khởi tạo các chân GPIO
  pinMode(MQ135_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  // Khởi tạo mutex
  sensorDataMutex = xSemaphoreCreateMutex();
  
  // Khởi tạo WiFi
  InitWiFi();
  
  // Khởi tạo OTA
  dhtDeviceIndex = addDevice("DHT20", "s958tymnfdgw3xmiyeo8", deviceCallback);
  Serial.printf("Added %d devices\n", deviceCount);
  
  // Tạo các task
  xTaskCreate(wifiTask, "WiFi_Task", 4096, NULL, 1, NULL);
  xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 2, NULL);
  xTaskCreate(readDHT20, "DHT20_Task", 4096, NULL, 2, NULL);
  xTaskCreate(readMQ135, "MQ135_Task", 2048, NULL, 1, NULL);
  xTaskCreate(ultrasonicTask, "Ultrasonic_Task", 4096, NULL, 3, NULL);
  xTaskCreate(pirTask, "PIR_Task", 2048, NULL, 2, NULL);
  xTaskCreate(otaTask, "OTA_Task", 8192, NULL, 2, NULL);
  
  Serial.println("Hệ thống đã khởi tạo xong và sẵn sàng!");
}

void loop()
{
  // Không cần làm gì ở đây vì tất cả đã được quản lý bởi FreeRTOS
  vTaskDelay(portMAX_DELAY);
}
