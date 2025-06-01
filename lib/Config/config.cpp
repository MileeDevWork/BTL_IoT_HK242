#include "config.hpp"

// Device configurations array
const DeviceConfig DEVICE_CONFIGS[] = {
    // Building device configuration
    {
        .token = "OrMees1ToDgts03u5TsV",
        .deviceType = DEVICE_TYPE_BUILDING,
        .deviceName = "Building_Control_System",
        
        // Pin configuration for Building device
        .pins = {
            .dhtPin = 8,           // DHT22 temperature sensor
            .mq135Pin = 1,         // Air quality sensor  
            .pirPin = 18,          // PIR motion sensor
            .pirPin2 = 19,         // Second PIR sensor
            .ultrasonicTrigPin = -1, // Not used
            .ultrasonicEchoPin = -1, // Not used
            .rfidSSPin = -1,       // Not used
            .rfidRSTPin = -1,      // Not used
            .relayPin = 21         // Lighting control relay
        },
        
        // Hardware features
        .hasRFID = false,
        .hasUltrasonic = false,
        .ultrasonicSlots = 0,
        
        // Environmental sensors
        .enableTempHumidity = true,
        .enableAirQuality = true,
        .enablePIR = true,  // People counting mode
        .enableLighting = true,
        
        // Intervals
        .envSensorInterval = 15000,  // 15 seconds
        .pirInterval = 5000,         // 5 seconds
        .ultrasonicInterval = 0      // Not used
    },
    
    // Carpark device configuration
    {
        .token = "WsyJtTftCGVWuGCnQ0OQ",
        .deviceType = DEVICE_TYPE_CARPARK,
        .deviceName = "Carpark_Management_System",
        
        // Pin configuration for Carpark device  
        .pins = {
            .dhtPin = 15,          // DHT22 temperature sensor
            .mq135Pin = 4,         // Air quality sensor
            .pirPin = 19,          // PIR motion sensor (security)
            .pirPin2 = 5,          // Second PIR sensor
            .ultrasonicTrigPin = 22, // Ultrasonic distance sensor TRIG
            .ultrasonicEchoPin = 18, // Ultrasonic distance sensor ECHO  
            .rfidSSPin = 16,       // RFID reader SS pin
            .rfidRSTPin = 21,      // RFID reader RST pin
            .relayPin = 17         // Gate control relay
        },
        
        // Hardware features
        .hasRFID = true,
        .hasUltrasonic = true,
        .ultrasonicSlots = 10,
        
        // Environmental sensors
        .enableTempHumidity = true,
        .enableAirQuality = true,
        .enablePIR = true,  // Security mode
        .enableLighting = true,
        
        // Intervals
        .envSensorInterval = 30000,  // 30 seconds (less frequent for parking)
        .pirInterval = 5000,         // 5 seconds
        .ultrasonicInterval = 5000   // 5 seconds
    }
};

// Current configuration pointer
const DeviceConfig* currentConfig = nullptr;

void initConfig() {
    // Deprecated - use initConfigFromDeviceId() instead
    currentConfig = &DEVICE_CONFIGS[0]; // Default to building
    Serial.println("WARNING: Using deprecated initConfig(). Use initConfigFromDeviceId() instead.");
}

void initConfigFromDeviceId(const char* deviceId) {
    int deviceMode = getDeviceModeFromId(deviceId);
    
    if (deviceMode < sizeof(DEVICE_CONFIGS) / sizeof(DeviceConfig)) {
        currentConfig = &DEVICE_CONFIGS[deviceMode];
        Serial.printf("Initialized device: %s (Type: %s) with ID: %s\n", 
                     currentConfig->deviceName, currentConfig->deviceType, deviceId);
        Serial.printf("Token: %s\n", currentConfig->token);
    } else {
        Serial.println("ERROR: Invalid device mode selected!");
        currentConfig = &DEVICE_CONFIGS[0]; // Default to building
    }
}

int getDeviceModeFromId(const char* deviceId) {
    String id = String(deviceId);
    id.toUpperCase();
    
    // Map device ID prefix to device mode
    if (id.startsWith("BLD") || id.startsWith("BUILDING")) {
        return 0; // Building
    } else if (id.startsWith("CPK") || id.startsWith("CARPARK") || id.startsWith("PARK")) {
        return 1; // Carpark
    }
    
    // Default to building if no match
    Serial.printf("WARNING: Unknown device ID prefix '%s', defaulting to Building mode\n", deviceId);
    return 0;
}

// Lấy cấu hình hiện tại của thiết bị
const DeviceConfig* getCurrentConfig() {
    return currentConfig;
}

// Kiểm tra tính năng có được kích hoạt không
bool isFeatureEnabled(const char* feature) {
    if (!currentConfig) return false;
      if (strcmp(feature, "temp_humidity") == 0) return currentConfig->enableTempHumidity;
    if (strcmp(feature, "air_quality") == 0) return currentConfig->enableAirQuality;
    if (strcmp(feature, "pir") == 0) return currentConfig->enablePIR;
    if (strcmp(feature, "lighting") == 0) return currentConfig->enableLighting;
    if (strcmp(feature, "rfid") == 0) return currentConfig->hasRFID;
    if (strcmp(feature, "ultrasonic") == 0) return currentConfig->hasUltrasonic;
    
    return false;
}

// Get pin configuration functions
int getDHTPin() {
    return currentConfig ? currentConfig->pins.dhtPin : -1;
}

int getMQ135Pin() {
    return currentConfig ? currentConfig->pins.mq135Pin : -1;
}

int getPIRPin() {
    return currentConfig ? currentConfig->pins.pirPin : -1;
}

int getPIRPin2() {
    return currentConfig ? currentConfig->pins.pirPin2 : -1;
}

int getUltrasonicTrigPin() {
    return currentConfig ? currentConfig->pins.ultrasonicTrigPin : -1;
}

int getUltrasonicEchoPin() {
    return currentConfig ? currentConfig->pins.ultrasonicEchoPin : -1;
}

int getRFIDSSPin() {
    return currentConfig ? currentConfig->pins.rfidSSPin : -1;
}

int getRFIDRSTPin() {
    return currentConfig ? currentConfig->pins.rfidRSTPin : -1;
}

int getRelayPin() {
    return currentConfig ? currentConfig->pins.relayPin : -1;
}

// Pin validation function
bool validatePinConfiguration() {
    if (!currentConfig) return false;
    
    const PinConfig* pins = &currentConfig->pins;
    
    Serial.println("=== PIN CONFIGURATION ===");
    Serial.printf("Device: %s\n", currentConfig->deviceType);
    
    if (currentConfig->enableTempHumidity && pins->dhtPin >= 0) {
        Serial.printf("DHT Sensor: GPIO %d\n", pins->dhtPin);
    }
    
    if (currentConfig->enableAirQuality && pins->mq135Pin >= 0) {
        Serial.printf("MQ135 Sensor: GPIO %d\n", pins->mq135Pin);
    }
    
    if (currentConfig->enablePIR && pins->pirPin >= 0) {
        Serial.printf("PIR Sensor: GPIO %d\n", pins->pirPin);
        if (pins->pirPin2 >= 0) {
            Serial.printf("PIR Sensor 2: GPIO %d\n", pins->pirPin2);
        }
    }
    
    if (currentConfig->hasUltrasonic && pins->ultrasonicTrigPin >= 0) {
        Serial.printf("Ultrasonic TRIG: GPIO %d\n", pins->ultrasonicTrigPin);
        Serial.printf("Ultrasonic ECHO: GPIO %d\n", pins->ultrasonicEchoPin);
    }
    
    if (currentConfig->hasRFID && pins->rfidSSPin >= 0) {
        Serial.printf("RFID SS: GPIO %d\n", pins->rfidSSPin);
        Serial.printf("RFID RST: GPIO %d\n", pins->rfidRSTPin);
    }
    
    if (currentConfig->enableLighting && pins->relayPin >= 0) {
        Serial.printf("Relay Control: GPIO %d\n", pins->relayPin);
    }
    
    Serial.println("========================");
    return true;
}
