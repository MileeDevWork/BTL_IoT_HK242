#include <wifi.hpp>

constexpr char WIFI_SSID[] = "ACLAB";
constexpr char WIFI_PASSWORD[] = "ACLAB2023";

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
    return true;
  }
  InitWiFi();
  return true;
}