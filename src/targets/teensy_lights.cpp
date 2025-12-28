#include <Arduino.h>
#include <FastLED.h>

#define LEFT_STRIP_PIN 0
#define RIGHT_STRIP_PIN 1
// #define NUM_LEDS 255
#define NUM_LEDS 300

const int ledPin = 13; // Pin number for the LED on Teensy 4.1
const int DELAY_INTERVAL_MS = 1000;

CRGB leftStrip[NUM_LEDS];
CRGB rightStrip[NUM_LEDS];

void setup()
{
    FastLED.addLeds<WS2812B, LEFT_STRIP_PIN, GRB>(leftStrip, NUM_LEDS);
    FastLED.addLeds<WS2812B, RIGHT_STRIP_PIN, GRB>(rightStrip, NUM_LEDS);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);
}

void loop()
{
    // Fill both strips with rainbow
    fill_rainbow(leftStrip, NUM_LEDS, 0, 1);
    fill_rainbow(rightStrip, NUM_LEDS, 0, 1);

    FastLED.show();
    digitalWrite(ledPin, HIGH);

    delay(DELAY_INTERVAL_MS);

    // Turn off
    FastLED.clear();
    FastLED.show();
    digitalWrite(ledPin, LOW);

    delay(DELAY_INTERVAL_MS);
}
