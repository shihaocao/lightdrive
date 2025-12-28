#include <Arduino.h>
#include <FastLED.h>

// For Teensy 4.x parallel output using WS2811_PORTA:
// Pin order: 19, 18, 14, 15, 17, 16, 22, 23 (strips 0-7)
// Move your wires: Strip 0 -> pin 19, Strip 1 -> pin 18

#define NUM_LEDS_PER_STRIP 300
#define NUM_STRIPS 2

const int ledPin = 13;
const int DELAY_INTERVAL_MS = 1000;

// Single contiguous array for parallel output
// Strip 0: leds[0] to leds[NUM_LEDS_PER_STRIP-1]
// Strip 1: leds[NUM_LEDS_PER_STRIP] to leds[2*NUM_LEDS_PER_STRIP-1]
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

void setup()
{
    // Parallel output on Teensy 4.x
    FastLED.addLeds<WS2811_PORTA, NUM_STRIPS, GRB>(leds, NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);
}

void loop()
{
    // Fill both strips with rainbow
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
        uint8_t hue = (i * 255) / NUM_LEDS_PER_STRIP;
        leds[i] = CHSV(hue, 255, 255);                          // Strip 0
        leds[i + NUM_LEDS_PER_STRIP] = CHSV(hue, 255, 255);     // Strip 1
    }

    FastLED.show();
    digitalWrite(ledPin, HIGH);

    delay(DELAY_INTERVAL_MS);

    // Turn off
    FastLED.clear();
    FastLED.show();
    digitalWrite(ledPin, LOW);

    delay(DELAY_INTERVAL_MS);
}
