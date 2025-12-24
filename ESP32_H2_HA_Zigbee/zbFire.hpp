#pragma once

#include <Zigbee.h>

#include "zbFastLED.hpp"

/* Zigbee binary sensor device configuration */
#define BINARY_DEVICE_ENDPOINT_NUMBER 1
ZigbeeBinary zbFire = ZigbeeBinary(BINARY_DEVICE_ENDPOINT_NUMBER);

void fireSwitch(bool state)
{
  if (state)
  {
    currentLedMode = LedMode::fire;
  }
  else
  {
    updateColorForAllLeds(0, 0, 0);
    currentLedMode = LedMode::light;
  }
}

void setupZBFire()
{
  // Set up binary status input + switch output
  zbFire.addBinaryOutput();
  zbFire.setBinaryOutputDescription("Feuer");
  zbFire.onBinaryOutputChange(fireSwitch);
  Zigbee.addEndpoint(&zbFire);
}

void updateFireSwitch()
{
  if (zbFire.getBinaryOutput() && currentLedMode != LedMode::fire)
  {
    zbFire.setBinaryOutput(false); // turn fire switch off
    zbFire.reportBinaryOutput();
  }
}
