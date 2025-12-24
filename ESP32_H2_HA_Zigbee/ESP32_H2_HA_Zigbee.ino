#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include <Zigbee.h>
#include <elapsedMillis.h>

#include "zbFastLED.hpp"
#include "zbLight.hpp"
#include "zbFire.hpp"

uint8_t bootButton = BOOT_PIN;
LedMode currentLedMode = LedMode::light;
elapsedSeconds timeSinceZigbeeNotConnected;
const unsigned long ZIGBEE_NOT_CONNECTED_TIMEOUT = 300; // s

void factoryResetIfBootIsPressed()
{
  // Checking button for factory reset
  if (digitalRead(bootButton) == LOW) // Push button pressed
  {
    // Key debounce handling
    delay(100);
    int startTime = millis();

    while (digitalRead(bootButton) == LOW) // Push button pressed
    {
      delay(50);

      if ((millis() - startTime) > 3000)
      {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        rgbLedWrite(ledBuildin, 0, 255, 0); // red
        delay(1000);
        rgbLedWrite(ledBuildin, 0, 0, 0);
        Zigbee.factoryReset();
      }
    }
  }
}

void setupZigbee()
{
  // When all EPs are registered, start Zigbee in End Device mode
  if (not Zigbee.begin(ZIGBEE_END_DEVICE))
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting in 1s...");
    rgbLedWrite(ledBuildin, 0, 255, 255); // purple
    delay(1000);
    rgbLedWrite(ledBuildin, 0, 0, 0);
    ESP.restart();
  }

  Serial.println("Connecting to network");

  timeSinceZigbeeNotConnected = 0;
  while (not Zigbee.connected())
  {
    Serial.print(".");
    rgbLedWrite(ledBuildin, 0, 0, 255); // blue
    delay(100);
    rgbLedWrite(ledBuildin, 0, 0, 0);
    delay(100);

    if (timeSinceZigbeeNotConnected >= ZIGBEE_NOT_CONNECTED_TIMEOUT)
    {
      Serial.println("Zigbee failed to connect!");
      Serial.println("Rebooting in 1s...");
      rgbLedWrite(ledBuildin, 255, 255, 0); // yellow or yellowish green
      delay(1000);
      rgbLedWrite(ledBuildin, 0, 0, 0);
      ESP.restart();
    }

    factoryResetIfBootIsPressed();
  }
}

void setup()
{
  Serial.begin(115200);
  elapsedMillis timeSinceSerialBegin;
  while (not Serial && timeSinceSerialBegin <= 5000) delay(100);

  // setup FastLED
  setupLeds();
  // Init RMT and leave light OFF
  rgbLedWrite(ledBuildin, 0, 0, 0);
  // Init button for factory reset
  pinMode(bootButton, INPUT_PULLUP);

  setupZBLight();
  setupZBFire();
  setupZigbee();

  rgbLedWrite(ledBuildin, 255, 0, 0); // green
  delay(500);
  rgbLedWrite(ledBuildin, 0, 0, 0);
}

void loop()
{
  updateLightState();
  updateFireSwitch();
  showLeds();
  factoryResetIfBootIsPressed();
}
