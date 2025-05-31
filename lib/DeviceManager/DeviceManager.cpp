#include "DeviceManager.hpp"

// Global instance
DeviceManager deviceManager;

DeviceManager::DeviceManager() {
    server = nullptr;
    provisioningMode = false;
}

DeviceManager::~DeviceManager() {
    if (server) {
        delete server;
    }
}

bool DeviceManager::isDeviceProvisioned() {
    prefs.begin("device", true);
    bool provisioned = prefs.getBool("provisioned", false);
    prefs.end();
    return provisioned;
}

DeviceProfile DeviceManager::getDeviceProfile() {
    DeviceProfile profile;
    
    prefs.begin("device", true);
    String deviceId = prefs.getString("device_id", "");
    String deviceType = prefs.getString("device_type", "");
    bool provisioned = prefs.getBool("provisioned", false);
    prefs.end();
    
    strcpy(profile.deviceId, deviceId.c_str());
    strcpy(profile.deviceType, deviceType.c_str());
    profile.isProvisioned = provisioned;
    
    return profile;
}

void DeviceManager::saveDeviceProfile(const DeviceProfile& profile) {
    prefs.begin("device", false);
    prefs.putString("device_id", profile.deviceId);
    prefs.putString("device_type", profile.deviceType);
    prefs.putBool("provisioned", profile.isProvisioned);
    prefs.end();
    
    Serial.printf("Device profile saved: ID=%s, Type=%s\n", 
                  profile.deviceId, profile.deviceType);
}

void DeviceManager::factoryReset() {
    prefs.begin("device", false);
    prefs.clear();
    prefs.end();
    Serial.println("Factory reset completed. Device will restart in provisioning mode.");
}

String DeviceManager::generateDefaultDeviceId() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return "DEV" + mac.substring(6); // Last 6 chars of MAC
}

bool DeviceManager::validateDeviceId(const String& deviceId) {
    // Simple validation: 3-15 chars, alphanumeric
    if (deviceId.length() < 3 || deviceId.length() > 15) {
        return false;
    }
    
    for (int i = 0; i < deviceId.length(); i++) {
        if (!isAlphaNumeric(deviceId.charAt(i))) {
            return false;
        }
    }
    return true;
}

int DeviceManager::getDeviceTypeFromId(const String& deviceId) {
    // Map device ID prefix to device type
    if (deviceId.startsWith("BLD") || deviceId.startsWith("BUILDING")) {
        return 0; // Building
    } else if (deviceId.startsWith("CPK") || deviceId.startsWith("CARPARK")) {
        return 1; // Carpark
    }
    
    // Default to building
    return 0;
}

void DeviceManager::startProvisioningMode() {
    provisioningMode = true;
    
    // Create AP for provisioning
    String apName = "ESP32-Setup-" + generateDefaultDeviceId();
    WiFi.softAP(apName.c_str(), "12345678");
    
    Serial.printf("Provisioning mode started. Connect to WiFi: %s (Password: 12345678)\n", apName.c_str());
    Serial.printf("Open browser and go to: http://192.168.4.1\n");
    
    // Start web server
    server = new WebServer(80);
    
    // Setup web interface
    server->on("/", [this]() {        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Device Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        .container { max-width: 400px; margin: 0 auto; }
        input, select, button { width: 100%; padding: 10px; margin: 5px 0; }
        button { background: #007bff; color: white; border: none; cursor: pointer; }
        button:hover { background: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Device Setup</h2>
        <form action="/save" method="POST">
            <label>Device ID:</label>
            <input type="text" name="deviceId" placeholder="e.g., BLD001, CPK001" required>
            
            <label>Device Type:</label>
            <select name="deviceType" required>
                <option value="building">Building Management</option>
                <option value="carpark">Carpark Management</option>
            </select>
            
            <button type="submit">Save Configuration</button>
        </form>
        
        <hr>
        <button onclick="location.href='/reset'">Factory Reset</button>
    </div>
</body>
</html>
        )";
        server->send(200, "text/html", html);
    });
      server->on("/save", HTTP_POST, [this]() {
        String deviceId = server->arg("deviceId");
        String deviceType = server->arg("deviceType");
        
        if (!validateDeviceId(deviceId)) {
            server->send(400, "text/plain", "Invalid Device ID format!");
            return;
        }
        
        DeviceProfile profile;
        strcpy(profile.deviceId, deviceId.c_str());
        strcpy(profile.deviceType, deviceType.c_str());
        profile.isProvisioned = true;
        
        saveDeviceProfile(profile);
        
        server->send(200, "text/html", 
            "<h2>Configuration Saved!</h2>"
            "<p>Device will restart in 3 seconds...</p>"
            "<script>setTimeout(function(){ window.close(); }, 3000);</script>");
        
        // Restart after 3 seconds
        delay(3000);
        ESP.restart();
    });
    
    server->on("/reset", [this]() {
        factoryReset();
        server->send(200, "text/html", 
            "<h2>Factory Reset Complete!</h2>"
            "<p>Device will restart in 3 seconds...</p>");
        delay(3000);
        ESP.restart();
    });
    
    server->begin();
}

void DeviceManager::stopProvisioningMode() {
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
    WiFi.softAPdisconnect(true);
    provisioningMode = false;
}

bool DeviceManager::isInProvisioningMode() {
    return provisioningMode;
}

void DeviceManager::handleProvisioningRequests() {
    if (server && provisioningMode) {
        server->handleClient();
    }
}
