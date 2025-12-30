#include <Arduino.h>
#include <OctoWS2811.h>
#include <FastLED.h>

// Teensy USB MIDI support is built-in when Tools -> USB Type includes MIDI

// Teensy 4.x can use any pins with OctoWS2811 via pin list
#define NUM_LEDS_PER_STRIP 300
#define NUM_STRIPS 2

const int BRIGHTNESS_CUTOFF = 30;
const int ledPin = 13;
const int NUM_LEDS_TOTAL = NUM_LEDS_PER_STRIP * NUM_STRIPS;

// Pin list: Strip 0 on pin 0, Strip 1 on pin 1
const byte pinList[NUM_STRIPS] = {0, 1};

// FastLED array
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// OctoWS2811 buffers (sized for actual strip count: numLeds * numStrips * 3 bytes / 4 + 1)
const int bufferSize = NUM_LEDS_PER_STRIP * NUM_STRIPS * 3 / 4 + 1;
DMAMEM int displayMemory[bufferSize];
int drawingMemory[bufferSize];

OctoWS2811 octo(NUM_LEDS_PER_STRIP, displayMemory, drawingMemory,
                WS2811_GRB | WS2811_800kHz, NUM_STRIPS, pinList);

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

float pulseStrength = 0.0f;

// Optional: keep track of last MIDI activity time if you later want timeouts
unsigned long lastMidiOnTime = 0;

static inline void handleMidi()
{
    // Drain all pending MIDI packets each loop iteration
    while (usbMIDI.read()) {
        const uint8_t type = usbMIDI.getType();

        // “Any MIDI on event only” interpreted as Note On with velocity > 0
        if (type == usbMIDI.NoteOn) {
            const uint8_t velocity = usbMIDI.getData2();
            if (velocity > 0) {
                pulseStrength = 255.0f;
                lastMidiOnTime = millis();
            }
        }

        // Ignore NoteOff and everything else per request
    }
}

void setup()
{
    pcontroller = new CTeensy4Controller<GRB>(&octo);
    FastLED.addLeds(pcontroller, leds, NUM_STRIPS * NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);
}

void loop()
{
    // Handle USB MIDI input (must be called frequently)
    handleMidi();

    // Exponential decay
    if (pulseStrength > 0.0f) {
        pulseStrength *= 0.9f;  // decay factor
    }
    if (pulseStrength < BRIGHTNESS_CUTOFF) {
        pulseStrength = 0.0f;
    }

    // Fill entire strip with solid color at current pulse strength
    const uint8_t brightness = (uint8_t)pulseStrength;
    const CRGB color = CHSV(0, 255, brightness);  // red at varying brightness
    fill_solid(leds, NUM_LEDS_TOTAL, color);

    FastLED.show();

    delay(1);
}
