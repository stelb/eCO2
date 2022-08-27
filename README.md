# eCO2 Sensor with SGP30 and M5Stamp C3

- platformio project
- color codes for eCO2 Values: green, green+yellow blinking, yellow, orange, red (blue blinking added if there is no baseline saved yet)
- first baselines will be saved after 12 hours and then every hour as preferences
- as stamp c3 is not supported directly by platformio, needed config is in platformio.ini
- SHT30 sensor supported (required right now actually..)
- Wifi support with wifimanager, not used yet

## TODO
- OTA updating (included in wfimanager?)
- expose data for scraping and graphing (MySensors?)
- reset baselines / wificonfig
- making SHT30 optional
- remember sensor serial and discard baseline on changed sensor
- support sht30 sensor (temparature/humidity/air pressure)

## Problems
Wifi and FastLED SK6812 is very unstable, so back to Adafruit neopixel.
