#pragma once

#include <elapsedMillis.h>
#include <FastLED.h>
#include <fx/1d/fire2012.h>
#include <fl/screenmap.h>

enum class LedMode : uint8_t
{
  light = 0,
  fire
};

extern LedMode currentLedMode;

elapsedMillis timeSinceLedShow;
const unsigned long LED_SHOW_INTERVAL = 66; // ms

#define LED_PIN     0
#define NUM_LEDS    79
#define BRIGHTNESS  64
CRGB leds[NUM_LEDS];

#define COOLING 75
#define SPARKING 50
fl::Fire2012 fire(NUM_LEDS, COOLING, SPARKING);

void updateColorForAllLeds(uint8_t in_red, uint8_t in_green, uint8_t in_blue)
{
  for (size_t index = 0; index < NUM_LEDS; ++index)
  {
    leds[index].setRGB(in_red, in_green, in_blue);
  }
}

void setupLeds()
{
  fl::ScreenMap screenMap = fl::ScreenMap::DefaultStrip(NUM_LEDS);
  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS).setScreenMap(screenMap).setCorrection(TypicalLEDStrip).setRgbw();
  FastLED.setBrightness(BRIGHTNESS);
  updateColorForAllLeds(0, 0, 0);
  FastLED.show();
}

void showLeds()
{
  if (timeSinceLedShow >= LED_SHOW_INTERVAL)
  {
    switch (currentLedMode)
    {
      case LedMode::fire: // zbFire.getBinaryOutput() == true
        fire.draw(fl::Fx1d::DrawContext(millis(), leds));
        break;
    }

    FastLED.show();
    timeSinceLedShow = 0;
  }
}
