#include <wifi.hpp>

constexpr char WIFI_SSID[] = "Min";
constexpr char WIFI_PASSWORD[] = "23230903";

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
  if (WiFi.status() != WL_CONNECTED)
  {
    InitWiFi();
    return WiFi.status() == WL_CONNECTED;
  }
  return true;
}