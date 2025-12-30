#include <Arduino.h>
#include <OctoWS2811.h>
#include <FastLED.h>

// Teensy 4.x can use any pins with OctoWS2811 via pin list
#define NUM_LEDS_PER_STRIP 300
#define NUM_STRIPS 2

const int BRIGHTNESS_CUTOFF = 30;
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
// Pulse state
float pulseStrength = 0;
uint8_t cycleCount = 0;
unsigned long lastTriggerTime = 0;

void setup()
{
    pcontroller = new CTeensy4Controller<GRB>(&octo);
    FastLED.addLeds(pcontroller, leds, NUM_STRIPS * NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);
}

void loop()
{
    unsigned long now = millis();

    // Trigger pulse every 1000ms
    if (now / 1000 != lastTriggerTime / 1000) {
        pulseStrength = 255.0f;
        lastTriggerTime = now;
    }

    // Exponential decay every 2 control cycles
    if (pulseStrength > 0) {
        pulseStrength *= 0.9f;  // decay factor
    }
    if (pulseStrength < BRIGHTNESS_CUTOFF) {
        pulseStrength = 0.0f;
    }

    // Fill entire strip with solid color at current pulse strength
    uint8_t brightness = (uint8_t)pulseStrength;
    CRGB color = CHSV(0, 255, brightness);  // red at varying brightness
    fill_solid(leds, NUM_LEDS_TOTAL, color);

    FastLED.show();

    delay(1);
}
