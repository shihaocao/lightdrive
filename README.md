# lightdrive
A task oriented music visualizer scheduler

## Background
I want to create an async task driven scheduler to help create light shows

## Principles
AI Usage:
- AI usage is ok, as long as we do not sacrifice learnings. Its biggest tools here will be cleaning things up, and bootstrapping.

## Installation Notes
1. Git clone
2. Install platformio extension into vscode.
3. Install teensy rules:
   1. https://www.pjrc.com/teensy/00-teensy.rules
4. Run `pio run -e teensy_blink -t upload`

## Debug Tools
```
ls /dev/tty*
```

# Dev Log
## 06/09 Start Project
I copied in my code somewhat from vvtol to bootstrap things. Going to first see if I can get
an anyio event loop to execute with sub ms resolution. I am making the big architectural gamble that says:
I have no requirement for the full event loop to be microcontroller only.

## 2055.12.28
Goals:
- I want to accurately control the LEDs around my room.
- I want to receive MIDI on the teensy, and send it to the light strip around my room.

Learnings:
- Things will start to fall apart once we have too many LEDs and we may have trouble reading from serial:
- https://github.com/FastLED/FastLED/wiki/Interrupt-problems
- ```
    fill_rainbow(rightStrip, NUM_LEDS, 0, 1); // Be careful if you ever feed a hue delta that's less than 1. Delta hue is a uint8_t.
    ```
Objectives:
- Determine if this is worth doing in the living room.

Notes:
- Right now my bedroom setup has two strips of 300. A WS2812 LED takes about `30us` to update one LED. Pushing down a whole strip of 300 down one wire will then take `9ms`. FastLED, unless configured will drive them sequentially meaning that two strips of `300` will take `18ms`. If I go to the living room with two strips of `600`, it will take `36ms`. Aka 30 fps. Unacceptable.
- https://github.com/FastLED/FastLED/wiki/Parallel-Output
- Let's try to do better

### Octo WS2811
Notes:
- https://www.pjrc.com/teensy/td_libs_OctoWS2811.html

## 2025.12.30

I will need this tomorrow:

```
lsusb | grep -i novation
aconnect -l
aseqdump -p "Launchpad X MIDI 1"
aconnect "Launchpad X MIDI 1" "Launchpad X MIDI 1"
```

Proof of concept:
```
python scripts/send_midi.py
pio run -e lightdrive3 -t upload
```
This will do:
```
Python -> MIDI -> (USB Host/MIDI/ALSA?) -> MIDI over USB -> Teensy -> LightDrive Program
```

Nice.