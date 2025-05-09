#ifndef HCSR04_H
#define HCSR04_H

#include <Arduino.h>

class UltraSonicDistanceSensor {
public:
    /**
     * Initialize the sensor with the specified trigger and echo pins.
     * @param triggerPin Digital pin for the trigger pin
     * @param echoPin Digital pin for the echo pin
     */
    UltraSonicDistanceSensor(int triggerPin, int echoPin);
    
    /**
     * Measures the distance in centimeters
     * @returns Distance in centimeters, or -1 if distance couldn't be measured
     */
    float measureDistanceCm();
    
private:
    int triggerPin;
    int echoPin;
    const unsigned long TIMEOUT_MICROS = 25000; // Maximum time to wait for echo (about 4m range)
};

#endif // HCSR04_H
