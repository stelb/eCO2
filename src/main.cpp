#include <Wire.h>
#include <Preferences.h>

#include <M5Unified.h>
#include "Adafruit_SGP30.h"
#include <FastLED.h>

Preferences prefs;

// timing
unsigned long now;

// LED
#define LED_DATA_PIN 2
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
// blinking
unsigned long lastBlink = 0;
#define BLINKINTERVAL 1000
uint8_t colorIndex = 0;
CRGB blinkPat[2];

// sgp30 TVOC
Adafruit_SGP30 sgp;

bool initialBaseline;
unsigned long lastRead = 0;
unsigned long nextSave;
uint16_t tvocBaseline, eco2Baseline;
// 12h+5m
#define FIRSTSAVE 1000 * 60 * 60 * 12 + 5 * 60 * 1000
// 1h
#define SAVEINTERVAL 1000 * 60 * 60
// 3s
#define READINTERVAL 3000

uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
  return absoluteHumidityScaled;
}

void setup() // 3s
{
  M5.begin();
  M5.Power.begin();

  FastLED.addLeds<SK6812, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(true);

  // Wire.begin(GPIO_NUM_1, GPIO_NUM_0);
  Wire.begin();

  Serial.begin(115200);
  Serial.println("SGP30 eCO2");

  if (!sgp.begin())
  {
    Serial.println("Sensor not found :(");
    //  while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  // stored prefs
  prefs.begin("eCO2");
  // do we have initial baseline?
  initialBaseline = prefs.getBool("init", false);
  if (initialBaseline)
  {
    // read existing baselines
    eco2Baseline = prefs.getUInt("eco2");
    tvocBaseline = prefs.getUInt("tvoc");
    // load into sensor
    sgp.setIAQBaseline(eco2Baseline, tvocBaseline);
    // save every hour
    nextSave = millis() + SAVEINTERVAL;
  }
  else
  {
    // first baseline after 12h
    nextSave = millis() + FIRSTSAVE; // now + 12h + 5min
  }
  blinkPat[0] = CRGB::Black;
  blinkPat[1] = CRGB::White;
  blinkPat[2] = CRGB::Blue;
}

void blink(CRGB col1, CRGB col2, CRGB col3)
{
  blinkPat[0] = col1;
  blinkPat[1] = col2;
  blinkPat[2] = col3;
}

void loop()
{
  now = millis();
  if (now - lastBlink > BLINKINTERVAL)
  {
    leds[0] = blinkPat[colorIndex];
    lastBlink = now;
    colorIndex = colorIndex == 2 ? 0 : colorIndex + 1;
  }
  if (millis() - lastRead > READINTERVAL)
  {
    lastRead = millis();
    if (!sgp.IAQmeasure())
    {
      Serial.println("Measurement failed");
      return;
    }
    if (sgp.eCO2 >= 1000 & sgp.eCO2 < 1250)
    {
      // leds[0] = CRGB::GreenYellow;
      blink(CRGB::Green, CRGB::Yellow, initialBaseline ? CRGB::Green : CRGB::Blue);
    }
    else if (sgp.eCO2 >= 1250 & sgp.eCO2 < 1750)
    {
      // leds[0] = CRGB::Yellow;
      blink(CRGB::Yellow, CRGB::Yellow, initialBaseline ? CRGB::Yellow : CRGB::Blue);
    }
    else if (sgp.eCO2 >= 1750 & sgp.eCO2 < 2000)
    {
      // leds[0] = CRGB::Orange;
      blink(CRGB::Orange, CRGB::Orange, initialBaseline ? CRGB::Orange : CRGB::Blue);
    }
    else if (sgp.eCO2 >= 2000)
    {
      // leds[0] = CRGB::Red;
      blink(CRGB::Red, CRGB::Red, initialBaseline ? CRGB::Red : CRGB::Blue);
    }
    else if (sgp.eCO2 < 1000)
    {
      // leds[0] = CRGB::Green;
      blink(CRGB::Green, CRGB::Green, initialBaseline ? CRGB::Green : CRGB::Blue);
    }
    /*
            CO2 ppm < 1000 : grün
    1000 <= CO2 ppm < 1250 : grün-gelb
    1250 <= CO2 ppm < 1750: gelb
    1750 <= CO2 ppm < 2000: gelb-rot
    2000 <= CO2 ppm: rot
    */

    Serial.print("TVOC ");
    Serial.print(sgp.TVOC);
    Serial.print(" ppb\t");
    Serial.print("eCO2 ");
    Serial.print(sgp.eCO2);
    Serial.println(" ppm");

    if (!sgp.IAQmeasureRaw())
    {
      Serial.println("Raw Measurement failed");
      return;
    }
    if (millis() > nextSave)
    {
      // save baseline, calc next save
      if (!sgp.getIAQBaseline(&eco2Baseline, &tvocBaseline))
      {
        Serial.print("get baselines failed!"); // handling?
      }
      prefs.putUInt("eco2", eco2Baseline);
      prefs.putUInt("tvoc", tvocBaseline);
      // prefs.end();
      nextSave = millis() + SAVEINTERVAL;
      // if first time: init true
      if (!initialBaseline)
      {
        prefs.putBool("init", true);
        initialBaseline = true;
      }
    }
    Serial.printf("Next Save in %lu minutes\n", (nextSave - millis()) / 1000 / 60);
    Serial.print("Raw H2 ");
    Serial.print(sgp.rawH2);
    Serial.print(" \t");
    Serial.print("Raw Ethanol ");
    Serial.print(sgp.rawEthanol);
    Serial.println("");
    FastLED.show();
  }
  FastLED.show();
  FastLED.delay(500);
}