# eCO2 Sensor with SGP30 and M5Stamp C3

- platformio project
- color codes for eCO2 Values: green, green+yellow blinking, yellow, orange, red (blue blinking added if there is no baseline saved yet)
- first baselines will be saved after 12 hours and then every hour as preferences
- as stamp c3 is not supported directly by platformio, needed config is in platformio.ini
- SHT30 sensor supported (required right now actually..)
- Wifi support with wifimanager, not used yet
- baseline & wifi config reset possible
- homeassistant mqtt autodiscovery
- mqtt config via wifimanager

## Reset config
There is only one button and one led, so:
 - start config mode: 3 buttonclicks in 2s (red blinking)
 - short click cycle through options
   - blue blinking: wifi reset
   - red, yellow blinking: baseline reset
   - green, red blinking: not used yet
 - activate option: press 2s
 - after this: normal operation

## TODO
- OTA updating (included in wfimanager?)
- expose data for scraping and graphing (MySensors?)
- making SHT30 optional
- remember sensor serial and discard baseline on changed sensor
- support sht30 sensor (temparature/humidity/air pressure)

## Problems
Wifi and FastLED SK6812 is very unstable, code completely removed
still some code cleanup needed
error handling
new code structure broke settings mode
