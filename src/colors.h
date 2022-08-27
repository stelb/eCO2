#ifdef USE_FASTLED
#define BLACK       CRGB::Black
#define RED         CRGB::Red
#define YELLOW      CRGB::Yellow
#define GREEN       CRGB::Green
#define BLUE        CRGB::Blue
#define ORANGE      CRGB::Orange
#define WHITE       CRGB::White
#else
#define BLACK       0x00000000
#define RED         0x00ff0000
#define YELLOW      0x00ffff00
#define GREEN       0x0000ff00
#define BLUE        0x000000ff
#define ORANGE      0x00ffa500
#define WHITE       0x00ffffff
#endif