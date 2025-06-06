
#include "control.hpp"

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);



void ledwhite_on()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(255, 255, 255));    
  NeoPixel.setPixelColor(1, NeoPixel.Color(255, 255, 255));
  NeoPixel.setPixelColor(2, NeoPixel.Color(255, 255, 255)); 
  NeoPixel.setPixelColor(3, NeoPixel.Color(255, 255, 255));  
  NeoPixel.show();
}

void ledwhite_off()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 0, 0));
  NeoPixel.setPixelColor(1, NeoPixel.Color(0, 0, 0));
  NeoPixel.setPixelColor(2, NeoPixel.Color(0, 0, 0)); 
  NeoPixel.setPixelColor(3, NeoPixel.Color(0, 0, 0));  
  NeoPixel.show();
}


