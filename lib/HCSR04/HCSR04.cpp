#include "HCSR04.h"

UltraSonicDistanceSensor::UltraSonicDistanceSensor(int triggerPin, int echoPin) {
    this->triggerPin = triggerPin;
    this->echoPin = echoPin;
    
    // Initialize pins
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    // Initialize trigger pin to low
    digitalWrite(triggerPin, LOW);
}

float UltraSonicDistanceSensor::measureDistanceCm() {
    // Clear the trigger pin
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    
    // Set the trigger pin high for 10 microseconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    
    // Read the echo pin: return sound wave travel time in microseconds
    unsigned long duration = pulseIn(echoPin, HIGH, TIMEOUT_MICROS);
    
    // If we timed out or got invalid value, return error
    if (duration == 0) {
        return -1;
    }
    
    // Calculate the distance: duration * speed of sound (343.2 m/s) / 2
    // Convert to cm: 343.2 * 100 / 1000000 / 2 = 0.01716
    return duration * 0.01716;
}
