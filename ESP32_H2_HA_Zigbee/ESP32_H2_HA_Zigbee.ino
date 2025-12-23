#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include <Zigbee.h>
#include <elapsedMillis.h>
#include <FastLED.h>

elapsedSeconds timeSinceZigbeeNotConnected;
const unsigned long ZIGBEE_NOT_CONNECTED_TIMEOUT = 300; // s

elapsedMillis timeSinceLedUpdate;
const unsigned long LED_UPDATE_INTERVAL = 20; // ms

#define LED_PIN     0
#define NUM_LEDS    79
#define BRIGHTNESS  64
CRGB leds[NUM_LEDS];

void updateColorForAllLeds(uint8_t in_red, uint8_t in_green, uint8_t in_blue)
{
  for (size_t index = 0; index < NUM_LEDS; ++index)
  {
    leds[index].setRGB(in_red, in_green, in_blue);
  }
}

uint8_t ledBuildin = RGB_BUILTIN;
uint8_t bootButton = BOOT_PIN;

/* Zigbee color dimmable light configuration */
#define ZIGBEE_RGB_LIGHT_ENDPOINT 10
ZigbeeColorDimmableLight zbColorLight = ZigbeeColorDimmableLight(ZIGBEE_RGB_LIGHT_ENDPOINT);

/* Zigbee binary sensor device configuration */
#define BINARY_DEVICE_ENDPOINT_NUMBER 1
ZigbeeBinary zbBinary = ZigbeeBinary(BINARY_DEVICE_ENDPOINT_NUMBER);

void binarySwitch(bool state)
{
  
}

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

void factoryResetifBootIsPressed()
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
        rgbLedWrite(ledBuildin, 0, 255, 0);
        delay(1000);
        rgbLedWrite(ledBuildin, 0, 0, 0);
        Zigbee.factoryReset();
      }
    }
  }
}

/********************* Arduino functions **************************/
void setup()
{
  Serial.begin(115200);

  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS).setRgbw();
  FastLED.setBrightness(BRIGHTNESS);
  updateColorForAllLeds(0, 0, 0);
  FastLED.show();

  // Init RMT and leave light OFF
  rgbLedWrite(ledBuildin, 0, 0, 0);
  // Init button for factory reset
  pinMode(bootButton, INPUT_PULLUP);

  // Enable both XY (RGB) and Temperature color capabilities
  uint16_t capabilities = ZIGBEE_COLOR_CAPABILITY_X_Y | ZIGBEE_COLOR_CAPABILITY_COLOR_TEMP;
  zbColorLight.setLightColorCapabilities(capabilities);
  // Set callback functions for RGB and Temperature modes
  zbColorLight.onLightChangeRgb(setRGBLight);
  zbColorLight.onLightChangeTemp(setTempLight);
  // Optional: Set callback function for device identify
  zbColorLight.onIdentify(identify);
  // Optional: Set Zigbee device name and model
  zbColorLight.setManufacturerAndModel("Espressif", "ZBColorLightBulb");
  // Set min/max temperature range (High Kelvin -> Low Mireds: Min and Max is switched)
  zbColorLight.setLightColorTemperatureRange(kelvinToMireds(6500), kelvinToMireds(2000));
  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbColorLight);

  // Set up binary status input + switch output
  zbBinary.addBinaryOutput();
  zbBinary.setBinaryOutputDescription("Switch");
  zbBinary.onBinaryOutputChange(binarySwitch);
  Zigbee.addEndpoint(&zbBinary);

  // When all EPs are registered, start Zigbee in End Device mode
  if (not Zigbee.begin(ZIGBEE_END_DEVICE))
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting in 1s...");
    rgbLedWrite(ledBuildin, 0, 255, 255);
    delay(1000);
    rgbLedWrite(ledBuildin, 0, 0, 0);
    ESP.restart();
  }

  Serial.println("Connecting to network");

  timeSinceZigbeeNotConnected = 0;
  while (not Zigbee.connected())
  {
    Serial.print(".");
    rgbLedWrite(ledBuildin, 0, 0, 255);
    delay(100);
    rgbLedWrite(ledBuildin, 0, 0, 0);
    delay(100);

    if (timeSinceZigbeeNotConnected >= ZIGBEE_NOT_CONNECTED_TIMEOUT)
    {
      Serial.println("Zigbee failed to connect!");
      Serial.println("Rebooting in 1s...");
      rgbLedWrite(ledBuildin, 255, 255, 0);
      delay(1000);
      rgbLedWrite(ledBuildin, 0, 0, 0);
      ESP.restart();
    }

    factoryResetifBootIsPressed();
  }

  Serial.println();

  rgbLedWrite(ledBuildin, 255, 0, 0);
  delay(500);
  rgbLedWrite(ledBuildin, 0, 0, 0);
}

void loop()
{
  if (timeSinceLedUpdate >= LED_UPDATE_INTERVAL)
  {
    FastLED.show();
    timeSinceLedUpdate = 0;
  }

  factoryResetifBootIsPressed();
}
