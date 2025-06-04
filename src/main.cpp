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
  ledStateQueue = xQueueCreate(1, sizeof(bool));
  pinMode(MQ135_PIN, INPUT);
  pinMode(pirPinIn, INPUT);
  pinMode(ledPin, OUTPUT);
  // xTaskCreate(readDHT11, "DHT20Task", 4096, NULL, 2, NULL);
  xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 1, NULL);
  xTaskCreate(ledControlTask, "LED Control Task", 2048, NULL, 3, NULL); 
  // xTaskCreate(peopleCountingTask, "peopleCountingTask", 4096, NULL, 2, NULL);
  // xTaskCreate(readMQ135, "MQ135Task", 2048, NULL, 2, NULL);
}

void loop()
{
}
