#include <Adafruit_NeoPixel.h>

#define BLINK_DATA_PIN 2
#define BLINK_LEDS 1

#define BLINK_NORMAL 1000
#define BLINK_FAST 300

namespace blinkNS
{
    Adafruit_NeoPixel strip(BLINK_LEDS, BLINK_DATA_PIN, NEO_GRB + NEO_KHZ800);

    uint16_t _interval = 1000;
    uint8_t index = 0;
    uint32_t pattern[3];

    void setup()
    {
        strip.setBrightness(40);
    }

    void set(uint32_t col1, uint32_t col2, uint32_t col3)
    {
        pattern[0] = col1;
        pattern[1] = col2;
        pattern[2] = col3;
    }

    void blinkInterval(uint16_t interval)
    {
        _interval = interval;
    }
    uint16_t getInterval()
    {
        return _interval;
    }

    void blink()
    {
        strip.setPixelColor(0, pattern[index]);
        strip.show();
        index = index == 2 ? 0 : index + 1;
    }

}
