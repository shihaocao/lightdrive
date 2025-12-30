#!/usr/bin/env python3
import time
import curses
import mido

KICK_NOTE = 36          # GM kick
CHANNEL = 9             # MIDI channel 10 is index 9 in most libraries
VELOCITY = 100
NOTE_OFF_DELAY_S = 0.05 # short gate; Teensy ignores off, but safe for others


def pick_output_port() -> str:
    ports = mido.get_output_names()
    if not ports:
        raise RuntimeError("No MIDI output ports found. Is the Teensy plugged in as USB MIDI?")

    # Prefer a port that looks like Teensy
    for p in ports:
        if "Teensy" in p or "teensy" in p:
            return p

    # Otherwise pick the first one
    return ports[0]


def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.keypad(True)

    port_name = pick_output_port()
    out = mido.open_output(port_name)

    stdscr.addstr(0, 0, f"MIDI out: {port_name}")
    stdscr.addstr(1, 0, "Press 'a' to send KICK (note 36, vel 100) on ch 10. Press 'q' to quit.")
    stdscr.refresh()

    while True:
        ch = stdscr.getch()
        if ch == -1:
            time.sleep(0.001)
            continue

        if ch in (ord('q'), ord('Q')):
            break

        if ch in (ord('a'), ord('A')):
            msg_on = mido.Message("note_on", note=KICK_NOTE, velocity=VELOCITY, channel=CHANNEL)
            out.send(msg_on)

            # Optional note_off (harmless even if Teensy ignores it)
            time.sleep(NOTE_OFF_DELAY_S)
            msg_off = mido.Message("note_off", note=KICK_NOTE, velocity=0, channel=CHANNEL)
            out.send(msg_off)

            stdscr.addstr(3, 0, f"Sent: {msg_on}            ")
            stdscr.refresh()


if __name__ == "__main__":
    curses.wrapper(main)
