#include <wifi.hpp>

constexpr char WIFI_SSID[] = "Min";
constexpr char WIFI_PASSWORD[] = "123456789";

// HÀM KẾT NỐI WIFI
void InitWiFi()
{
  Serial.println("Đang kết nối WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Kiểm tra kết nối WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nĐã kết nối WiFi!");
}

// KIỂM TRA VÀ KẾT NỐI LẠI WIFI NẾU MẤT KẾT NỐI
bool reconnect()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }
  InitWiFi();
  return true;
}

// Task quản lý WiFi
void wifiTask(void *pvParameters) {
    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            InitWiFi();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}