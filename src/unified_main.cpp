// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/queue.h>
// #include "DHT20.h"
// #include "HCSR04.h"
// #include "DeviceManager.hpp"

// // Pin Definitions
// #define SDA_PIN GPIO_NUM_11
// #define SCL_PIN GPIO_NUM_12
// #define LED_PIN 48
// #define SONIC_TRIGGER GPIO_NUM_6
// #define SONIC_ECHO GPIO_NUM_7

// // WiFi Credentials
// const char* WIFI_SSID = "Min";
// const char* WIFI_PASSWORD = "123456789";

// // ThingsBoard Configuration
// const char* THINGSBOARD_SERVER = "app.coreiot.com";
// const uint16_t THINGSBOARD_PORT = 1883;
// const uint32_t MAX_MESSAGE_SIZE = 1024;

// // Device Tokens
// const char* ULTRASONIC_TOKEN = "q6zmf5kxiqqd8p4qf4g6";
// const char* DHT_TOKEN = "mae15of5vf8oc2v3bdap";

// // Global Objects
// DHT20 dht20;
// UltraSonicDistanceSensor ultrasonic(SONIC_TRIGGER, SONIC_ECHO);
// DeviceManager deviceManager(THINGSBOARD_SERVER, THINGSBOARD_PORT);
// QueueHandle_t ledStateQueue;

// // Device Indices
// int dhtDeviceIndex = -1;
// int ultrasonicDeviceIndex = -1;
// bool objectDetected = false;

// // Prototypes
// void wifiTask(void *pvParameters);
// void deviceManagerTask(void *pvParameters);
// void sensorTask(void *pvParameters);
// void ultrasonicTask(void *pvParameters);

// // Connect to WiFi network
// void connectWifi() {
//     Serial.print("Connecting to WiFi...");
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("Connected to WiFi");
// }

// // WiFi management task
// void wifiTask(void *pvParameters) {
//     for (;;) {
//         if (WiFi.status() != WL_CONNECTED) {
//             connectWifi();
//         }
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

// // Device connection management task
// void deviceManagerTask(void *pvParameters) {
//     for (;;) {
//         for (int i = 0; i < 2; i++) { // We have 2 devices: DHT and Ultrasonic
//             int deviceIndex = (i == 0) ? dhtDeviceIndex : ultrasonicDeviceIndex;
//             if (deviceIndex >= 0) {
//                 deviceManager.connectDeviceToThingsBoard(deviceIndex);
//             }
//         }
        
//         deviceManager.processDevices();
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// // DHT sensor reading task
// void sensorTask(void *pvParameters) {
//     vTaskDelay(pdMS_TO_TICKS(5000)); // Initial delay
    
//     for (;;) {
//         dht20.read();
//         float temperature = dht20.getTemperature();
//         float humidity = dht20.getHumidity();
        
//         if (!isnan(humidity) && !isnan(temperature)) {
//             Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temperature, humidity);
            
//             if (dhtDeviceIndex >= 0 && deviceManager.isDeviceConnected(dhtDeviceIndex)) {
//                 // Publish sensor data
//                 String payload = "{\"temperature\":" + String(temperature) + 
//                                 ",\"humidity\":" + String(humidity) + "}";
//                 deviceManager.publishTelemetry(dhtDeviceIndex, payload);
//             }
//         } else {
//             Serial.println("Failed to read from DHT20 sensor!");
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(10000)); // Read every 10 seconds
//     }
// }

// // Ultrasonic sensor task
// void ultrasonicTask(void *pvParameters) {
//     const float DETECTION_THRESHOLD = 10.0; // 10cm threshold
//     bool previousState = false;
//     const char* ENTITY_ID = "6892f6a0-1426-11f0-b943-e12b9c63441a"; // Entity ID for parking spot
//     const char* SPOT_NAME = "spot_A1"; // Spot identifier
    
//     // Initial delay to ensure system is ready
//     vTaskDelay(pdMS_TO_TICKS(5000));
    
//     for (;;) {
//         // Measure distance in cm
//         float distance = ultrasonic.measureDistanceCm();
        
//         // Check if valid reading
//         if (distance >= 0) {
//             // Determine if object is detected (distance < 10cm)
//             bool currentState = (distance < DETECTION_THRESHOLD);
            
//             // Print current distance
//             Serial.printf("Distance: %.2f cm, Object detected: %s\n", 
//                          distance, currentState ? "true" : "false");
            
//             // Only send telemetry when state changes to reduce network traffic
//             if (currentState != previousState && 
//                 ultrasonicDeviceIndex >= 0 && 
//                 deviceManager.isDeviceConnected(ultrasonicDeviceIndex)) {
                
//                 // Get current timestamp (milliseconds since epoch)
//                 unsigned long timestamp = millis(); // Using millis as placeholder
                
//                 // Format the payload according to the specified structure
//                 String payload = "{";
//                 payload += "\"entityId\":\"" + String(ENTITY_ID) + "\",";
//                 payload += "\"timeseries\":[";
//                 payload += "{";
//                 payload += "\"ts\":" + String(timestamp) + ",";
//                 payload += "\"values\":{";
//                 payload += "\"" + String(SPOT_NAME) + "\":\"" + String(currentState ? "true" : "false") + "\"";
//                 payload += "}";
//                 payload += "}";
//                 payload += "]";
//                 payload += "}";
                
//                 deviceManager.publishTelemetry(ultrasonicDeviceIndex, payload);
                
//                 previousState = currentState;
//                 objectDetected = currentState;
                
//                 Serial.println("Sent telemetry: " + payload);
//             }
//         } else {
//             Serial.println("Error reading from ultrasonic sensor!");
//         }
        
//         // Check every 5 seconds
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

// void setup() {
//     // Initialize serial and hardware
//     Serial.begin(115200);
//     Wire.begin(SDA_PIN, SCL_PIN);
//     pinMode(LED_PIN, OUTPUT);
//     digitalWrite(LED_PIN, LOW);
//     dht20.begin();
    
//     // Create queues
//     ledStateQueue = xQueueCreate(1, sizeof(bool));
    
//     // Add devices to the manager
//     dhtDeviceIndex = deviceManager.addDevice("DHT20", DHT_TOKEN);
//     ultrasonicDeviceIndex = deviceManager.addDevice("Ultrasonic", ULTRASONIC_TOKEN);
    
//     Serial.printf("Added DHT device at index %d\n", dhtDeviceIndex);
//     Serial.printf("Added Ultrasonic device at index %d\n", ultrasonicDeviceIndex);
    
//     // Create tasks
//     xTaskCreate(wifiTask, "WiFi Task", 4096, NULL, 1, NULL);
//     xTaskCreate(deviceManagerTask, "Device Manager", 8192, NULL, 2, NULL);
//     xTaskCreate(sensorTask, "Sensor Task", 4096, NULL, 3, NULL);
//     xTaskCreate(ultrasonicTask, "Ultrasonic Task", 4096, NULL, 3, NULL);
// }

// void loop() {
//     // Nothing to do here as FreeRTOS tasks handle everything
//     vTaskDelay(portMAX_DELAY);
// }
