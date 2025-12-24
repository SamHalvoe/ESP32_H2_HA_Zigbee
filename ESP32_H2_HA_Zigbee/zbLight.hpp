#pragma once

#include <Zigbee.h>

#include "zbFastLED.hpp"

uint8_t ledBuildin = RGB_BUILTIN;

/* Zigbee color dimmable light configuration */
#define ZIGBEE_RGB_LIGHT_ENDPOINT 10
ZigbeeColorDimmableLight zbColorLight = ZigbeeColorDimmableLight(ZIGBEE_RGB_LIGHT_ENDPOINT);

/********************* Temperature conversion functions **************************/
uint16_t kelvinToMireds(uint16_t kelvin)
{
  return 1000000 / kelvin;
}

uint16_t miredsToKelvin(uint16_t mireds)
{
  return 1000000 / mireds;
}

/********************* RGB LED functions **************************/
void setRGBLight(bool state, uint8_t red, uint8_t green, uint8_t blue, uint8_t level)
{
  if (not state)
  {
    rgbLedWrite(ledBuildin, 0, 0, 0);
    updateColorForAllLeds(0, 0, 0);
    return;
  }

  currentLedMode = LedMode::light;
  float brightness = static_cast<float>(level) / 255.0f;
  rgbLedWrite(ledBuildin, green * brightness, red * brightness, blue * brightness);
  updateColorForAllLeds(red * brightness, green * brightness, blue * brightness);
}

/********************* Temperature LED functions **************************/
void setTempLight(bool state, uint8_t level, uint16_t mireds)
{
  if (not state)
  {
    rgbLedWrite(ledBuildin, 0, 0, 0);
    updateColorForAllLeds(0, 0, 0);
    return;
  }

  currentLedMode = LedMode::light;
  float brightness = static_cast<float>(level) / 255.0f;
  // Convert mireds to color temperature (K) and map to white/yellow
  uint16_t kelvin = miredsToKelvin(mireds);
  uint8_t warm = constrain(map(kelvin, 2000, 6500, 255, 0), 0, 255);
  uint8_t cold = constrain(map(kelvin, 2000, 6500, 0, 255), 0, 255);
  rgbLedWrite(ledBuildin, warm * brightness, warm * brightness, cold * brightness);
  updateColorForAllLeds(warm * brightness, warm * brightness, cold * brightness);
}

// Create a task on identify call to handle the identify function
void identify(uint16_t time)
{
  static uint8_t blink = 1;

  if (time == 0)
  {
    // If identify time is 0, stop blinking and restore light as it was used for identify
    zbColorLight.restoreLight();

    if (zbColorLight.getLightState())
    {
      updateColorForAllLeds(zbColorLight.getLightRed(), zbColorLight.getLightGreen(), zbColorLight.getLightBlue());
    }
    else
    {
      updateColorForAllLeds(0, 0, 0);
    }

    return;
  }

  rgbLedWrite(ledBuildin, 255 * blink, 255 * blink, 255 * blink);
  updateColorForAllLeds(255 * blink, 255 * blink, 255 * blink);
  blink = (blink == 1 ? 0 : 1);
}

void setupZBLight()
{
  // Enable both XY (RGB) and Temperature color capabilities
  uint16_t capabilities = ZIGBEE_COLOR_CAPABILITY_X_Y | ZIGBEE_COLOR_CAPABILITY_COLOR_TEMP;
  zbColorLight.setLightColorCapabilities(capabilities);
  // Set callback functions for RGB and Temperature modes
  zbColorLight.onLightChangeRgb(setRGBLight);
  zbColorLight.onLightChangeTemp(setTempLight);
  // Optional: Set callback function for device identify
  zbColorLight.onIdentify(identify);
  // Optional: Set Zigbee device name and model
  zbColorLight.setManufacturerAndModel("Halvoe", "ColorEffectLight");
  // Set min/max temperature range (High Kelvin -> Low Mireds: Min and Max is switched)
  zbColorLight.setLightColorTemperatureRange(kelvinToMireds(6500), kelvinToMireds(2000));
  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbColorLight);
}

void updateLightState()
{
  if (zbColorLight.getLightState() && currentLedMode != LedMode::light)
  {
    zbColorLight.setLightState(false); // turn normal light off
  }
}
