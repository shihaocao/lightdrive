#!/usr/bin/env python3
import time
import curses
import mido

PPQN = 24  # MIDI Clock pulses per quarter note


def pick_output_port() -> str:
    ports = mido.get_output_names()
    if not ports:
        raise RuntimeError("No MIDI output ports found. Is the Teensy plugged in as USB MIDI?")

    # Prefer Teensy if present
    for p in ports:
        if "Teensy" in p or "teensy" in p:
            return p

    return ports[0]


def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.keypad(True)

    bpm = 120.0
    running = False

    port_name = pick_output_port()
    out = mido.open_output(port_name)

    stdscr.addstr(0, 0, f"MIDI out: {port_name}")
    stdscr.addstr(1, 0, "s = Start | t = Stop | +/- = BPM | q = Quit")
    stdscr.refresh()

    next_tick = time.monotonic()

    while True:
        ch = stdscr.getch()

        if ch in (ord('q'), ord('Q')):
            if running:
                out.send(mido.Message("stop"))
            break

        elif ch in (ord('s'), ord('S')):
            out.send(mido.Message("start"))
            running = True
            next_tick = time.monotonic()

        elif ch in (ord('t'), ord('T')):
            out.send(mido.Message("stop"))
            running = False

        elif ch == ord('+'):
            bpm += 1.0

        elif ch == ord('-'):
            bpm = max(20.0, bpm - 1.0)

        # Emit MIDI clock if running
        if running:
            seconds_per_tick = 60.0 / (bpm * PPQN)
            now = time.monotonic()
            if now >= next_tick:
                out.send(mido.Message("clock"))
                next_tick += seconds_per_tick

        stdscr.addstr(3, 0, f"BPM: {bpm:6.1f} | Running: {running}      ")
        stdscr.refresh()

        time.sleep(0.0005)


if __name__ == "__main__":
    curses.wrapper(main)
