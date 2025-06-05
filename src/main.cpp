#include <Arduino.h>
#include <Wire.h>
#include <wifi.hpp>
#include <global.hpp>
#include <mqtt.hpp>
#include <sensor.hpp>
#include <config.hpp>
#include <DeviceManager.hpp>

// Cài đặt và khởi tạo hệ thống
void setup()
{
  Serial.begin(115200);
  delay(5000);
  
  //for scheduler task
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Tắt LED ban đầu
  ledStateQueue = xQueueCreate(1, sizeof(bool)); // Hàng đợi kích thước 1, lưu bool
  // mqttClient.setCallback(mqttCallback);

  Serial.println("ESP32 IoT Device Starting...");
  // Check if device is provisioned
  if (!deviceManager.isDeviceProvisioned()) {
    Serial.println("Device not provisioned. Starting provisioning mode...");
    deviceManager.startProvisioningMode();
    
    // Wait for provisioning (device will restart after provisioning)
    while (!deviceManager.isDeviceProvisioned()) {
      deviceManager.handleProvisioningRequests();
      delay(100);
    }
  }
  
  // Get device profile and initialize configuration
  DeviceProfile profile = deviceManager.getDeviceProfile();
  initConfigFromDeviceId(profile.deviceId);  const DeviceConfig* config = getCurrentConfig();
    Serial.printf("Starting %s with Device ID: %s\n", config->deviceName, profile.deviceId);
  InitWiFi();
  sensorDataMutex = xSemaphoreCreateMutex();
  if (config->enableTempHumidity) {
    xTaskCreate(readDHT11, "DHT20_Task", 4096, NULL, 2, NULL);
  }
  
  if (config->enableAirQuality) {
    xTaskCreate(readMQ135, "MQ135_Task", 2048, NULL, 1, NULL);
  }
  
  if (config->enablePIR) {
    xTaskCreate(pirTask, "PIR_Task", 2048, NULL, 1, NULL);
    xTaskCreate(peopleCountingTask, "PeopleCounting_Task", 2048, NULL, 1, NULL);
  }
  
  if (config->hasUltrasonic) {
    xTaskCreate(carslotTask, "CarSlot_Task", 4096, NULL, 1, NULL);
  }
  
  if (config->hasRFID) {
    xTaskCreate(rfidTask, "RFID_Task", 2048, NULL, 1, NULL);
  }
  
  // Always create ThingsBoard task
  xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 2, NULL);
  xTaskCreate(ledControlTask, "LED Control Task", 2048, NULL, 1, NULL);
  
  
  Serial.printf("Device %s initialized successfully!\n", config->deviceType);
}

// Vòng lặp chính - kiểm tra lệnh reset
void loop()
{

}