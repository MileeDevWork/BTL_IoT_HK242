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
  
  pinMode(0, INPUT_PULLUP);
  if (config->enableTempHumidity) {
    xTaskCreate(readDHT20, "DHT20_Task", 4096, NULL, 2, NULL);
  }
  
  if (config->enableAirQuality) {
    xTaskCreate(readMQ135, "MQ135_Task", 2048, NULL, 1, NULL);
  }
  
  if (config->enablePIR) {
    xTaskCreate(pirTask, "PIR_Task", 2048, NULL, 1, NULL);
  }
  
  if (config->hasUltrasonic) {
    xTaskCreate(carslotTask, "CarSlot_Task", 4096, NULL, 1, NULL);
  }
  
  if (config->hasRFID) {
    xTaskCreate(rfidTask, "RFID_Task", 2048, NULL, 1, NULL);
  }
  
  // Always create ThingsBoard task
  xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 2, NULL);
  
  Serial.printf("Device %s initialized successfully!\n", config->deviceType);
}

// Vòng lặp chính - kiểm tra lệnh reset
void loop()
{
  // Check for factory reset command via Serial
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command.equalsIgnoreCase("FACTORY_RESET") || command.equalsIgnoreCase("RESET")) {
      Serial.println("Factory reset initiated via Serial command...");
      deviceManager.factoryReset();
      delay(1000);
      ESP.restart();
    }
  }
  
  // Check for hardware factory reset button (GPIO 0 pressed for 5 seconds)
  static unsigned long buttonPressStart = 0;
  static bool buttonPressed = false;
  
  if (digitalRead(0) == LOW) { // Button pressed (active LOW)
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressStart = millis();
      Serial.println("Reset button pressed...");
    } else if (millis() - buttonPressStart > 5000) { // Held for 5 seconds
      Serial.println("Factory reset initiated via button...");
      deviceManager.factoryReset();
      delay(1000);
      ESP.restart();
    }
  } else {
    if (buttonPressed && millis() - buttonPressStart < 5000) {
      Serial.println("Reset button released (too early)");
    }
    buttonPressed = false;
  }
  
  delay(100);
}