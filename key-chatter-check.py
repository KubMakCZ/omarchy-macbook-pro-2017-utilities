#!/usr/bin/env python3
"""Zjistí, proč se na vestavěné klávesnici zdvojují písmena.

Čte události klávesnice a vypíše POUZE souhrn podezřelých kláves:
  odskok  = nový stisk téže klávesy do --bounce ms po uvolnění (vada spínače)
  podržení = stisk delší než --hold ms (klávesa se uvolňuje pomalu a opakuje se)
Napsaný text se neukládá ani nevypisuje. Vyžaduje sudo.

Použití: sudo python3 key-chatter-check.py [--seconds 60] [--bounce 40] [--hold 250]
"""
import argparse
import collections
import re
import select
import struct
import sys
import time

EVENT = struct.Struct("llHHi")
NAMES = {
    16: "Q", 17: "W", 18: "E", 19: "R", 20: "T", 21: "Y/Z", 22: "U", 23: "I", 24: "O", 25: "P",
    30: "A", 31: "S", 32: "D", 33: "F", 34: "G", 35: "H", 36: "J", 37: "K", 38: "L",
    44: "Z/Y", 45: "X", 46: "C", 47: "V", 48: "B", 49: "N", 50: "M",
    14: "Backspace", 28: "Enter", 57: "mezerník",
}
MODIFIERS = {29, 42, 54, 56, 97, 100, 125, 126, 464}


def find_keyboard():
    with open("/proc/bus/input/devices", encoding="utf-8", errors="replace") as devices:
        for block in devices.read().split("\n\n"):
            if 'Name="Apple SPI Keyboard"' in block:
                match = re.search(r"\bevent\d+\b", block)
                if match:
                    return "/dev/input/" + match.group(0)
    return None


def main():
    parser = argparse.ArgumentParser(description="Souhrn odskoků a dlouhých stisků kláves.")
    parser.add_argument("--device", help="výchozí: Apple SPI Keyboard")
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--bounce", type=int, default=40, help="hranice odskoku v ms")
    parser.add_argument("--hold", type=int, default=250, help="hranice dlouhého stisku v ms")
    args = parser.parse_args()
    device = args.device or find_keyboard()
    if not device:
        sys.exit("Klávesnice Apple SPI nebyla nalezena; zadej --device /dev/input/eventN.")
    try:
        source = open(device, "rb", buffering=0)
    except PermissionError:
        sys.exit("Chybí oprávnění; spusť příkaz pomocí sudo.")

    print(f"Měřím {args.seconds} s na {device}. Piš běžný text, hlavně problémová písmena.")
    print("Text se neukládá. Ukončení: Ctrl+C.")
    pressed, released = {}, {}
    presses = collections.Counter()
    bounces = collections.defaultdict(list)
    holds = collections.defaultdict(list)
    end = time.monotonic() + args.seconds
    try:
        while (remaining := end - time.monotonic()) > 0:
            if not select.select([source], [], [], remaining)[0]:
                break
            data = source.read(EVENT.size)
            if len(data) != EVENT.size:
                break
            seconds, micros, kind, code, value = EVENT.unpack(data)
            if kind != 1 or code in MODIFIERS:
                continue
            now = seconds * 1000 + micros / 1000
            if value == 1:
                presses[code] += 1
                if code in released and now - released[code] < args.bounce:
                    bounces[code].append(now - released[code])
                pressed[code] = now
            elif value == 0 and code in pressed:
                duration = now - pressed.pop(code)
                if duration > args.hold:
                    holds[code].append(duration)
                released[code] = now
    except KeyboardInterrupt:
        pass

    print(f"\nZaznamenáno stisků: {sum(presses.values())}")
    if not bounces and not holds:
        print("Nic podezřelého. Zkus delší měření nebo víc problémových písmen.")
        return
    for title, found, unit in (("Odskoky (stisk hned po uvolnění)", bounces, "nejkratší"),
                               ("Dlouhé stisky (hrozí automatické opakování)", holds, "nejdelší")):
        if not found:
            continue
        print(f"\n{title}:")
        for code, values in sorted(found.items(), key=lambda item: -len(item[1])):
            edge = min(values) if unit == "nejkratší" else max(values)
            print(f"  {NAMES.get(code, f'kód {code}'):>10}: {len(values)}x z {presses[code]} stisků, "
                  f"{unit} {edge:.0f} ms")
    print("\nOdskoky znamenají vadu spínače (typická pro klávesnice 2016-2019).")
    print("Dlouhé stisky řeší delší prodleva opakování kláves v Hyprlandu.")


if __name__ == "__main__":
    main()
