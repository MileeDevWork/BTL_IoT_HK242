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
  pinMode(MQ135_PIN, INPUT);
  pinMode(pirPin, INPUT);

  xTaskCreate(ultrasonicTask, "Ultrasonic_Task", 4096, NULL, 1, NULL);
  xTaskCreate(carslotTask, "CarSlot_Task", 4096, NULL, 1, NULL);
  xTaskCreate(pirTask, "PIR_Task", 2048, NULL, 1, NULL);
  // xTaskCreate(readDHT20, "DHT20_Task", 4096, NULL, 2, NULL);
  // xTaskCreate(readDHT11, "DHT20Task", 4096, NULL, 2, NULL);
  // xTaskCreate(TaskThingsBoard, "ThingsBoard_Task", 4096, NULL, 2, NULL);
  // xTaskCreate(readMQ135, "MQ135Task", 2048, NULL, 1, NULL);
  // xTaskCreate(peopleCountingTask, "PeopleCounting", 4096, NULL, 1, NULL);
}

void loop()
{
}
