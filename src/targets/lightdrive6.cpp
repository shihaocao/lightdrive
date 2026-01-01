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
CRGBSet stripLeft(leds, NUM_LEDS_PER_STRIP);
CRGBSet stripRight(leds, NUM_LEDS_PER_STRIP * 2 - 1, NUM_LEDS_PER_STRIP - 1);  // reversed
// CRGBSet stripRight(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP * 2);  // reversed

// OctoWS2811 buffers (sized for actual strip count: numLeds * numStrips * 3 bytes / 4 + 1)
const int bufferSize = NUM_LEDS_PER_STRIP * NUM_STRIPS * 3 / 4 + 1;
DMAMEM int displayMemory[bufferSize];
int drawingMemory[bufferSize];

OctoWS2811 octo(NUM_LEDS_PER_STRIP, displayMemory, drawingMemory,
                WS2811_GRB | WS2811_800kHz, NUM_STRIPS, pinList);

// Custom controller to bridge FastLED to OctoWS2811
template <EOrder RGB_ORDER = GRB>
class CTeensy4Controller : public CPixelLEDController<RGB_ORDER, 8, 0xFF>
{
    OctoWS2811 *pocto;

public:
    CTeensy4Controller(OctoWS2811 *_pocto) : pocto(_pocto) {}

    virtual void init() override { pocto->begin(); }

    virtual void showPixels(PixelController<RGB_ORDER, 8, 0xFF> &pixels) override
    {
        uint32_t i = 0;
        while (pixels.has(1))
        {
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

const int kMaxAnimations = 100;
const int kMaxAnimationFrames = 1000;
const int kLastAnimationFrame = kMaxAnimationFrames - 1;
int note_to_ani_tbl[kMaxAnimations] = {}; // This maps from MIDI Note to the Animation Index
int NULL_ANIMATION_IDX = 0;
int KICK_EVENT_BASE = 1;
int KICK_EVENT_IDX = 1;
int SNARE_EVENT_BASE = 17;
int SNARE_EVENT_IDX = 17;
int BREATHING_MIN = 40;
int BREATHING_EVENT_BASE = BREATHING_MIN;
int BREATHING_IDX = 80;
int BREATHING_MAX = 80;

int WAVE_EVENT_BASE = 60;

bool animation_running[kMaxAnimations] = {};                     // Used for tracking if the animation is running. Used for FX
int animation_tick[kMaxAnimations] = {};                         // This maps from animation to the current frame index.
CRGB animation_lookup[kMaxAnimations][kMaxAnimationFrames] = {}; // this is a 2d array. Rows are animations, cols are the frames within each animation
int pulse_position[kMaxAnimations] = {};

const int kMaxSubPadColorCount = 16;
CHSV kick_colors[kMaxSubPadColorCount] = {};
CHSV snare_colors[kMaxSubPadColorCount] = {};

// Launchpad X Custom note numbers
const int LOWER_LEFT_START = 36;
const int TOP_LEFT_START = 52;
const int LOWER_RIGHT_START = 68;
const int TOP_RIGHT_START = 84;
const int PAD_COUNT = 16; // 4 x 4 grid.

void setupEventLookups()
{
    // Map the lookups
    for (int i = 0; i < LOWER_LEFT_START; i++)
    {
        note_to_ani_tbl[i] = NULL_ANIMATION_IDX; // technically invalid
    }
    for (int i = LOWER_LEFT_START; i < LOWER_LEFT_START + PAD_COUNT; i++)
    {
        int sub_idx = i - LOWER_LEFT_START;
        note_to_ani_tbl[i] = WAVE_EVENT_BASE + sub_idx;
    }
    for (int i = LOWER_RIGHT_START; i < LOWER_RIGHT_START + PAD_COUNT; i++)
    {
        int sub_idx = i - LOWER_RIGHT_START;
        note_to_ani_tbl[i] = BREATHING_EVENT_BASE + sub_idx;
    }
    for (int i = TOP_LEFT_START; i < TOP_LEFT_START + PAD_COUNT; i++)
    {
        int sub_idx = i - TOP_LEFT_START;
        note_to_ani_tbl[i] = KICK_EVENT_BASE + sub_idx;
    }
    for (int i = TOP_RIGHT_START; i < TOP_RIGHT_START + PAD_COUNT; i++)
    {
        int sub_idx = i - TOP_RIGHT_START;
        note_to_ani_tbl[i] = SNARE_EVENT_BASE + sub_idx;
    }
}

void setupAnimations()
{
    // INITIALIZE THE ANIMATIONS

    // Start all animation ticks at the last frame
    for (int i = 0; i < kMaxAnimations; i++)
    {
        animation_tick[i] = kLastAnimationFrame;
    }

    // WARNING:
    // IT IS ACTUALLY G R B not RGB!!!
    // CRGB(0, val, 0) = red!
    // CHSV 25 is green ish
    // 250 is also green ish
    // 150 is purple ish
    // 80 is yellow ish

    for(int i = 0; i < kMaxSubPadColorCount; i++) {
        uint8_t color_hue = 30*(i / 2);
        kick_colors[i] = CHSV(color_hue, 255, 255);
    }

    for(int i = 0; i < kMaxSubPadColorCount; i++) {
        uint8_t color_hue = 30*(i / 2);
        snare_colors[i] = CHSV(color_hue, 255, 255);
    }

    // set the idx 0 1 [2, 3] to white, i don't like that yellow.
    snare_colors[2] = CHSV(0, 0, 255);
    snare_colors[3] = CHSV(0, 0, 255);
    kick_colors[2] = CHSV(0, 0, 255);
    kick_colors[3] = CHSV(0, 0, 255);

    // kick animations
    for (int c = 0; c < kMaxSubPadColorCount; c++) {
        int kick_event_idx = c + KICK_EVENT_BASE;
        CHSV kick_color = kick_colors[c];

        uint8_t val = 255;
        for (int i = 0; i < kMaxAnimationFrames; i++)
        {
            val = (uint8_t)(val * 0.92f);
            if (val < 10)
            {
                val = 0;
            }

            kick_color.val = val;
            // animation_lookup[KICK_EVENT_IDX][i] = CRGB(0, val, 0);
            animation_lookup[kick_event_idx][i] = kick_color;
        }
    }

    // snare animations
    for (int c = 0; c < kMaxSubPadColorCount; c++) {
        int event_idx = c + SNARE_EVENT_BASE;
        CHSV ani_color = snare_colors[c];

        uint8_t val = 255;
        for (int i = 0; i < kMaxAnimationFrames; i++)
        {
            const uint8_t strobe_frame_max = 3;
            uint8_t white_intensity = i < strobe_frame_max ? 255 : 0;
            ani_color.val = white_intensity;
            animation_lookup[event_idx][i] = ani_color;
        }
    }

    for (int c = 0; c < kMaxSubPadColorCount; c++) {
        CHSV ani_color = kick_colors[c]; // re use the kick colors lol
        int event_idx = c + BREATHING_EVENT_BASE;

        for (uint16_t i = 0; i < kMaxAnimationFrames; i++)
        {
            const uint16_t breathing_period = 100; // frames per cycle
            const uint8_t  breathing_min    = 40;
            const uint8_t  breathing_max    = 245;

            // Phase within cycle
            uint16_t phase = i % breathing_period;

            uint8_t intensity;

            // First half: ramp up
            if (phase < breathing_period / 2) {
                intensity = map(
                    phase,
                    0, breathing_period / 2,
                    breathing_min, breathing_max
                );
            }
            // Second half: ramp down
            else {
                intensity = map(
                    phase,
                    breathing_period / 2, breathing_period,
                    breathing_max, breathing_min
                );
            }

            ani_color.val = intensity;
            animation_lookup[event_idx][i] = ani_color;
        }
    }

    // The last frame of every animation shall be zero
    for (int i = 0; i < kMaxAnimations; i++)
    {
        animation_lookup[i][kLastAnimationFrame] = CRGB(0, 0, 0);
    }
}

void setupSerial()
{
    Serial.begin(115200);

    uint32_t t0 = millis();
    while (!Serial && (millis() - t0) < 1500)
    {
    }

    Serial.println("Boot: USB MIDI + Serial active");
}

void setup()
{
    pcontroller = new CTeensy4Controller<GRB>(&octo);
    FastLED.addLeds(pcontroller, leds, NUM_STRIPS * NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    pinMode(ledPin, OUTPUT);

    setupEventLookups();
    setupAnimations();
    setupSerial();
}

struct MidiEvent
{
    uint8_t type;
    uint8_t note;
    uint8_t velocity;
    uint32_t timestamp_ms;
};

MidiEvent lastMidiEvent;

bool isEffect(int idx) {
    return (idx >= BREATHING_MIN) && (idx <= BREATHING_MAX);
    // return idx == BREATHING_IDX;
    // pad idx based, WRONG
    // return (idx >= LOWER_RIGHT_START) && (idx < (LOWER_RIGHT_START + PAD_COUNT));
}

static inline void handleMidi()
{
    while (usbMIDI.read())
    {
        lastMidiEvent.type = usbMIDI.getType();
        lastMidiEvent.note = usbMIDI.getData1();
        lastMidiEvent.velocity = usbMIDI.getData2();
        lastMidiEvent.timestamp_ms = millis();

        // get the note mapping:
        if (lastMidiEvent.type == usbMIDI.NoteOn)
        {
            int ani_idx = note_to_ani_tbl[lastMidiEvent.note];
            animation_running[ani_idx] = true; // Shouldn't matter as long as effect is < 100 ms.
            animation_tick[ani_idx] = 0;
            Serial.printf("Note on ani_idx=%d\n", ani_idx);
            continue; // ignore everything that isn't note on for now.
        }

        if (lastMidiEvent.type == usbMIDI.NoteOff) 
        {
            int ani_idx = note_to_ani_tbl[lastMidiEvent.note];
            animation_running[ani_idx] = false; // Shouldn't matter as long as effect is < 100 ms.

            // End the effect immidiaetely if it's in the FX region.
            if (isEffect(ani_idx))
            {
                animation_running[ani_idx] = false;
                animation_tick[ani_idx] = kLastAnimationFrame;
            }
            Serial.printf("Note off ani_idx=%d\n", ani_idx);
            continue;
        }
    }
}

static inline void overlayMovingPulse(CRGB* leds, int ani_idx)
{
    const uint8_t speed_mult = 6;
    const uint8_t pulseWidth = 40;

    int& pulsePos = pulse_position[ani_idx]; // Make this per animation
    pulsePos += speed_mult;
    if (pulsePos >= NUM_LEDS_PER_STRIP) {
        pulsePos = 0;
    }

    for (uint8_t i = 0; i < pulseWidth; i++)
    {
        int pos = pulsePos + i;
        if (pos >= NUM_LEDS_PER_STRIP) break;

        uint8_t v = 255 - (uint16_t(i) * 255 / pulseWidth);
        CHSV color = kick_colors[ani_idx - WAVE_EVENT_BASE]; // THIS IS A HACK AND IS NOT PORTABLE
        color.v = v;

        // CRGB += CRGB is saturating inside FastLED
        stripLeft[pos]         += color; // strip 0
        stripRight[pos]        += color; // strip 1
    }
}

static inline void satAddPixel(CRGB& dst, const CRGB& src)
{
    dst.r = qadd8(dst.r, src.r);
    dst.g = qadd8(dst.g, src.g);
    dst.b = qadd8(dst.b, src.b);
}

void loop()
{
    // Handle USB MIDI input (must be called frequently)
    handleMidi();

    // for any non zero animation_tick, tick it forwards by 1
    for (int i = 0; i < kMaxAnimations; i++)
    {
        if (animation_tick[i] < kLastAnimationFrame)
        { // dont go over the last frame lol
            animation_tick[i] += 1;
        }
    }

    // for all running animations, if it is running, and at the end, go back.
    for (int i = 0; i < kMaxAnimations; i++)
    {
        if (!animation_running && isEffect(i))
        {
            animation_tick[i] = kMaxAnimationFrames; // Go to end
        }
    }

    // Mix animations -> base_color (saturating)
    CRGB base_color = CRGB(0, 0, 0);
    for (int ani_idx = 0; ani_idx < kMaxAnimations; ani_idx++)
    {
        int frame_idx = animation_tick[ani_idx];

        // If you ever use sentinel values, clamp/skip here
        if (frame_idx < 0) continue;
        if (frame_idx > kLastAnimationFrame) frame_idx = kLastAnimationFrame;

        CRGB ani_value = animation_lookup[ani_idx][frame_idx];

        // saturating add (do NOT rely on operator+= doing what you want)
        satAddPixel(base_color, ani_value);
    }

    // Background
    fill_solid(leds, NUM_LEDS_TOTAL, base_color);

    // Status LED
    int status = 0;
    if(base_color.r > 0 || base_color.g > 0 || base_color.b > 0) {
        status += 1;
    }

    for(int ani_idx = WAVE_EVENT_BASE; ani_idx < WAVE_EVENT_BASE + kMaxSubPadColorCount; ani_idx++) {
        if(animation_running[ani_idx]) {
            status += 1;
            overlayMovingPulse(leds, ani_idx);
        }
    }

    // Status LED
    uint8_t pin_signal = status ? HIGH : LOW;
    digitalWrite(ledPin, pin_signal);

    // Serial.printf("Brightness %u\n", brightness);
    FastLED.show();

    delay(1);
}
