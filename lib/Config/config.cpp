#include "config.hpp"

// Device configurations array
const DeviceConfig DEVICE_CONFIGS[] = {
    // Building device configuration
    {
        .token = "OrMees1ToDgts03u5TsV",
        .deviceType = DEVICE_TYPE_BUILDING,
        .deviceName = "Building_Control_System",
        
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
