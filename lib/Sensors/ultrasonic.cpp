#include <sensor.hpp>

// Thêm khai báo cho HC-SR04 và ultrasonic task
UltraSonicDistanceSensor* ultrasonicSensor = NULL;
bool objectDetected = false;

// Task đọc cảm biến siêu âm HC-SR04
void ultrasonicTask(void *pvParameters) {
    const float DETECTION_THRESHOLD = 10.0; // 10cm threshold
    bool previousState = false;
    const char* ENTITY_ID = "6892f6a0-1426-11f0-b943-e12b9c63441a"; // Entity ID for parking spot
    const char* SPOT_NAME = "spot_A1"; // Spot identifier
    
    // Initial delay to ensure system is ready
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    for (;;) {
        if (ultrasonicSensor == NULL) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        // Measure distance in cm
        float distance = ultrasonicSensor->measureDistanceCm();
        
        // Check if valid reading
        if (distance >= 0) {
            // Determine if object is detected (distance < 10cm)
            bool currentState = (distance < DETECTION_THRESHOLD);
            
            // Print current distance
            Serial.printf("Distance: %.2f cm, Object detected: %s\n", 
                         distance, currentState ? "true" : "false");
            
            // Only send telemetry when state changes to reduce network traffic
            if (currentState != previousState && tb.connected()) {
                tb.sendTelemetryData(SPOT_NAME, currentState ? "true" : "false");
                previousState = currentState;
                objectDetected = currentState;
                
                Serial.printf("Sent parking spot status: %s\n", currentState ? "occupied" : "free");
            }
        } else {            Serial.println("Error reading from ultrasonic sensor!");
        }
        
        // Cập nhật mỗi 5 giây theo yêu cầu
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
