// #i nclude <esp_task_wdt.h>

#include <WiFiManager.h>
#include <Preferences.h>
#include <EasyButton.h>

#include <FastLED.h>

#include "colors.h"
#include "blink.h"
#include "sensor.h"
#include "settings.h"

char sensorID[23];

// Button
#define BUTTON_PIN 3
EasyButton button(BUTTON_PIN);

// prefs
Preferences prefs;

// 3s
#define READINTERVAL 3000

void wifi(void *parameter)
{
  WiFiManager wifiManager;
  Preferences prefs;

  prefs.begin("mqtt");
  prefs.getString("server", "").c_str();
  prefs.getString("port", "1883");

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server",
                                          prefs.getString("server", "").c_str(),
                                          40);
  wifiManager.addParameter(&custom_mqtt_server);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port",
                                        prefs.getString("port", "1883").c_str(), 5);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.autoConnect(sensorID);

  prefs.putString("server", custom_mqtt_server.getValue());
  prefs.putString("port", custom_mqtt_port.getValue());
  prefs.end();
  Serial.printf("server: %s port: %s", custom_mqtt_server.getValue(),
                custom_mqtt_port.getValue());
  // vTaskDelete(NULL);
}

bool setMode = false;
bool setLevel = false;
int setting = 0;

void sensorsTask(void *parameter)
{
  for (;;)
  {
    if (!setMode)
      sensor::read();
    Serial.println(uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(READINTERVAL / portTICK_PERIOD_MS);
  }
}

void blinkTask(void *parameter)
{
  for (;;)
  {
    // if (!setMode)
    blinkNS::blink();
    vTaskDelay(blinkNS::getInterval() / portTICK_PERIOD_MS);
  }
}

void enterSetup()
{
  setMode = true;
  blinkNS::blinkInterval(200);
  blinkNS::set(BLACK, RED, BLACK);
}

void settings()
{
  setLevel = true;
  if (button.pressedFor(2000))
  {
    switch (setting)
    {
    case 1:
      // configure wifi
      wifi(NULL);
      break;
    case 2:
      sensor::startsgp(true);
      break;
    case 3:
      // exit
      break;
    default:
      break;
    }
    setMode = false;
    setLevel = false;
    blinkNS::blinkInterval(1000);
    blinkNS::set(WHITE, BLACK, ORANGE);
    return;
  }
  if (button.wasReleased())
  {
    setting++;
    switch (setting)
    {
    case 1:
      blinkNS::set(BLACK, BLUE, BLACK);
      break;
    case 2:
      blinkNS::set(RED, YELLOW, BLACK);
      break;
    case 3:
      blinkNS::set(GREEN, RED, GREEN);
    default:
      setting = 0; // back to first option
      break;
    }
  }
}

void setup() // 3s
{
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);

  snprintf(sensorID, 23, "eCO2-%04X%08X", chip, (uint32_t)chipid);

  Serial.begin(115200);
  // M5.begin();

  button.begin();
  button.onSequence(3, 2000, enterSetup);

  // pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("eCO2 Sensor");
  Serial.println(sensorID);

  // xTaskCreate(wifi, "wifimanager", 4000, NULL, 1, NULL);

  // LED setup
  blinkNS::setup();

  // startup blink colors
  blinkNS::set(ORANGE, WHITE, BLUE);

  wifi(NULL);

  sensor::setup(sensorID);
  // start sensor readings task
  xTaskCreate(sensorsTask, "sensors", 4000, NULL, 1, NULL);
  // start blinking
  xTaskCreate(blinkTask, "blinking", 1024, NULL, 1, NULL);
}

void loop()
{
  button.read();
  //
  if (!setMode && button.pressedFor(10000))
  {
    Serial.println("longpress");
    blinkNS::set(BLACK, RED, BLACK);
    setMode = true;
  }

  if (!setMode)
  {
    EVERY_N_MILLIS(READINTERVAL)
    {
      //      sensor::read();
    }
  }
  else
  {
    if (setLevel || button.releasedFor(50))
      settings();
  }
}