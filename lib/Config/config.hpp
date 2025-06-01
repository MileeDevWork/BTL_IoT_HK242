#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pin configuration structure for device-specific pin mappings
typedef struct {
    int dhtPin;
    int mq135Pin;
    int pirPin;
    int pirPin2;
    int ultrasonicTrigPin;
    int ultrasonicEchoPin;
    int rfidSSPin;
    int rfidRSTPin;
    int relayPin;
} PinConfig;

// Cấu trúc cấu hình thiết bị
typedef struct {
    const char* token;
    const char* deviceType;
    const char* deviceName;
    
    // Pin configuration for this device
    PinConfig pins;
    
    // Hardware features
    bool hasRFID;
    bool hasUltrasonic;
    int ultrasonicSlots;
    
    // Environmental sensors
    bool enableTempHumidity;
    bool enableAirQuality;
    bool enablePIR;
    bool enableLighting;
    
    // Intervals
    uint32_t envSensorInterval;
    uint32_t pirInterval;
    uint32_t ultrasonicInterval;
} DeviceConfig;

#define DEVICE_TYPE_BUILDING "building"
#define DEVICE_TYPE_CARPARK "carpark"
// #define CURRENT_DEVICE_MODE 1  // Deprecated - now using NVS device ID

extern const DeviceConfig DEVICE_CONFIGS[];
extern const DeviceConfig* currentConfig;

// Khởi tạo cấu hình
void initConfig();
void initConfigFromDeviceId(const char* deviceId);
int getDeviceModeFromId(const char* deviceId);
// Lấy cấu hình hiện tại
const DeviceConfig* getCurrentConfig();
// Kiểm tra tính năng
bool isFeatureEnabled(const char* feature);

// Pin access functions
int getDHTPin();
int getMQ135Pin();
int getPIRPin();
int getPIRPin2();
int getUltrasonicTrigPin();
int getUltrasonicEchoPin();
int getRFIDSSPin();
int getRFIDRSTPin();
int getRelayPin();
bool validatePinConfiguration();

#ifdef __cplusplus
}
#endif

#endif // CONFIG_HPP
