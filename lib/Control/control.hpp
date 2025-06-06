#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <global.hpp>

#ifdef __cplusplus
extern "C"
{
#endif

    extern Adafruit_NeoPixel NeoPixel;

    void ledwhite_on();
    void ledwhite_off();

#ifdef __cplusplus
}
#endif
#endif