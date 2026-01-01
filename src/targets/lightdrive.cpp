#include <Arduino.h>
#include <OctoWS2811.h>
#include <FastLED.h>

// Teensy 4.x can use any pins with OctoWS2811 via pin list
#define NUM_LEDS_PER_STRIP 300
#define NUM_STRIPS 2

const int ledPin = 13;
const int DELAY_INTERVAL_MS = 1000;
const int NUM_LEDS_TOTAL = NUM_LEDS_PER_STRIP * NUM_STRIPS;

// Pin list: Strip 0 on pin 0, Strip 1 on pin 1
const byte pinList[NUM_STRIPS] = {0, 1};

// FastLED array
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// OctoWS2811 buffers (sized for actual strip count: numLeds * numStrips * 3 bytes / 4 + 1)
const int bufferSize = NUM_LEDS_PER_STRIP * NUM_STRIPS * 3 / 4 + 1;
DMAMEM int displayMemory[bufferSize];
int drawingMemory[bufferSize];

OctoWS2811 octo(NUM_LEDS_PER_STRIP, displayMemory, drawingMemory, WS2811_GRB | WS2811_800kHz, NUM_STRIPS, pinList);

// Custom controller to bridge FastLED to OctoWS2811
template <EOrder RGB_ORDER = GRB>
class CTeensy4Controller : public CPixelLEDController<RGB_ORDER, 8, 0xFF> {
    OctoWS2811 *pocto;
public:
    CTeensy4Controller(OctoWS2811 *_pocto) : pocto(_pocto) {}

    virtual void init() override { pocto->begin(); }

    virtual void showPixels(PixelController<RGB_ORDER, 8, 0xFF> &pixels) override {
        uint32_t i = 0;
        while (pixels.has(1)) {
            uint8_t r = pixels.loadAndScale0();
            uint8_t g = pixels.loadAndScale1();
            uint8_t b = pixels.loadAndScale2();
            pocto->setPixel(i++, r, g, b);
            pixels.stepDithering();
            pixels.advanceData();
        }
        pocto->show();
    }
};

CTeensy4Controller<GRB> *pcontroller;

// CRGBSet views for symmetric addressing from center
// stripLeft[0] = center, stripLeft[299] = far left
// stripRight[0] = center, stripRight[299] = far right (reversed in memory)
CRGBSet stripLeft(leds, NUM_LEDS_PER_STRIP);
CRGBSet stripRight(leds, NUM_LEDS_PER_STRIP * 2 - 1, NUM_LEDS_PER_STRIP - 1);  // reversed

int pulsePos = 0;

void setup()
{
    pcontroller = new CTeensy4Controller<GRB>(&octo);
    FastLED.addLeds(pcontroller, leds, NUM_STRIPS * NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);
}

void loop()
{
    // Clear previous frame
    FastLED.clear();

    // Draw a pulse that travels outward from center on both strips symmetrically
    const int speed_mult = 4;
    pulsePos += speed_mult;
    if (pulsePos >= NUM_LEDS_PER_STRIP) {
        pulsePos = 0;
    }

    const int pulseWidth = 20;
    for (int i = 0; i < pulseWidth; i++) {
        int pos = pulsePos + i;
        if (pos < NUM_LEDS_PER_STRIP) {
            uint8_t brightness = 255 - (i * 255 / pulseWidth);  // fade tail
            stripLeft[pos] = CHSV(0, 255, brightness);   // red pulse going left
            stripRight[pos] = CHSV(0, 255, brightness);  // red pulse going right (mirrored)
        }
    }

    FastLED.show();

    delay(1);  // ~100 fps
}
