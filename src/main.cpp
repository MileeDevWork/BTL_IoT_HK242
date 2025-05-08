#include <Arduino.h>
#include <Wire.h>
#include <wifi.hpp>
#include <global.hpp>
#include <mqtt.hpp> 
#include <sensor.hpp>

void setup()
{
  Serial.begin(115200);
  dht.begin();
  InitWiFi();
  sensorDataMutex = xSemaphoreCreateMutex();
  xTaskCreate(readDHT11, "DHT20Task", 4096, NULL, 2, NULL);
  xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 2, NULL);
}

void loop()
{
}
