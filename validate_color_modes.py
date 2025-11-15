#!/usr/bin/env python3
import re
import sys
from pathlib import Path

def segment_by_mode(lines):
    segments = []
    current = {"mode": "unknown", "lines": []}
    for ln in lines:
        if ln.startswith("ColorMode="):
            if current["lines"]:
                segments.append(current)
            current = {"mode": ln.strip().split("=",1)[1], "lines": []}
        else:
            current["lines"].append(ln)
    if current["lines"]:
        segments.append(current)
    return segments

def analyze_segment(seg):
    text = "".join(seg["lines"]) if isinstance(seg["lines"], list) else seg["lines"]
    tc = len(re.findall(r"\x1B\[38;2;(\d{1,3});(\d{1,3});(\d{1,3})m", text))
    ansi = len(re.findall(r"\x1B\[(3\d|9\d)m", text))
    esc_present = "\x1b" in text
    return {"mode": seg["mode"], "truecolor": tc, "ansi": ansi, "esc": esc_present}

def main():
    log = Path('staging_run.log')
    if not log.exists():
        print('missing staging_run.log')
        sys.exit(1)
    lines = log.read_text(encoding='utf-8', errors='ignore').splitlines(True)
    segments = segment_by_mode(lines)
    if not segments:
        print('no color mode segments found')
        sys.exit(1)
    results = [analyze_segment(s) for s in segments]
    for r in results:
        print(f"mode={r['mode']} tc={r['truecolor']} ansi={r['ansi']} esc={'YES' if r['esc'] else 'NO'}")
    # Assertions
    has_mono = any(r['mode'] == 'mono' for r in results)
    has_tc = any(r['mode'] == 'truecolor' for r in results)
    has_ansi = any(r['mode'] == 'ansi' for r in results)
    if not (has_mono and has_tc and has_ansi):
        print('missing one or more modes')
        sys.exit(2)
    for r in results:
        if r['mode'] == 'mono' and (r['truecolor'] > 0 or r['ansi'] > 0):
            print('mono contains ANSI escapes')
            sys.exit(3)
        if r['mode'] == 'truecolor' and r['truecolor'] == 0:
            print('truecolor segment has no sequences')
            sys.exit(4)
        if r['mode'] == 'ansi' and r['ansi'] == 0:
            print('ansi segment has no sequences')
            sys.exit(5)
    print('ok')

if __name__ == '__main__':
    main()
