#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cấu trúc cấu hình thiết bị
typedef struct {
    const char* token;
    const char* deviceType;
    const char* deviceName;
    bool hasRFID;
    bool hasUltrasonic;
    int ultrasonicSlots;
    bool enableTempHumidity;
    bool enableAirQuality;
    bool enablePIR;
    bool enableLighting;
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

#ifdef __cplusplus
}
#endif

#endif // CONFIG_HPP
