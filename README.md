# lightdrive
A task oriented music visualizer scheduler

## Background
I want to create an async task driven scheduler to help create light shows

## Principles
AI Usage:
- AI usage is ok, as long as we do not sacrifice learnings. Its biggest tools here will be cleaning things up, and bootstrapping.

# Dev Log
## 06/09 Start Project
I copied in my code somewhat from vvtol to bootstrap things. Going to first see if I can get
an anyio event loop to execute with sub ms resolution. I am making the big architectural gamble that says:
I have no requirement for the full event loop to be microcontroller only.

## 2055.12.28
Goals:
- I want to accurately control the LEDs around my room.
- I want to receive MIDI on the teensy, and send it to the light strip around my room.

Objectives:
- Determine if this is worth doing in the living room.
