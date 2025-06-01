#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device profile structure
struct DeviceProfile {
    char deviceId[16];        // "BLD001", "CPK001"
    char deviceType[16];      // "building", "carpark"
    bool isProvisioned;       // Setup completed flag
};

class DeviceManager {
private:
    Preferences prefs;
    WebServer* server;
    bool provisioningMode;
    
public:
    DeviceManager();
    ~DeviceManager();
    
    // Core functions
    bool isDeviceProvisioned();
    DeviceProfile getDeviceProfile();
    void saveDeviceProfile(const DeviceProfile& profile);
    void factoryReset();
    
    // Provisioning functions
    void startProvisioningMode();
    void stopProvisioningMode();
    bool isInProvisioningMode();
    void handleProvisioningRequests();
    
    // Utility functions
    String generateDefaultDeviceId();
    bool validateDeviceId(const String& deviceId);
    int getDeviceTypeFromId(const String& deviceId);
};

// Global instance
extern DeviceManager deviceManager;

#ifdef __cplusplus
}
#endif

#endif // DEVICE_MANAGER_HPP
