#ifndef WIFI_HPP
#define WIFI_HPP
#include <WiFi.h>

#ifdef __cplusplus
extern "C" {
#endif
// //

void InitWiFi();
bool reconnect();
void wifiTask(void *pvParameters);


///
#ifdef __cplusplus
}
#endif

#endif