#pragma once

namespace blinkNS
{
    void setup();
    void set(uint32_t col1, uint32_t col2, uint32_t col3);
    void blink();
    void blinkInterval(uint16_t interval);
    uint16_t getInterval();
}