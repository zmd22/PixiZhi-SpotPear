#!/usr/bin/env python3
"""Regenerate the project's original PCM tones using Python's standard library."""
from pathlib import Path
import math
import struct
import wave

out = Path(__file__).resolve().parents[1] / 'sounds'
out.mkdir(exist_ok=True)
rate = 16000
patterns = {
    'chime': [(660, 0.12), (880, 0.18), (1100, 0.24)],
    'alarm': [(740, 0.25), (1000, 0.25)] * 4,
}
for name, notes in patterns.items():
    data = bytearray()
    for freq, duration in notes:
        count = int(rate * duration)
        for i in range(count):
            envelope = max(0, min(1, i / (rate * 0.012), (count - 1 - i) / (rate * 0.02)))
            sample = int(7000 * envelope * math.sin(2 * math.pi * freq * i / rate))
            data.extend(struct.pack('<h', sample))
        data.extend(bytes(int(rate * 0.045) * 2))
    with wave.open(str(out / (name + '.wav')), 'wb') as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(rate)
        sound.writeframes(data)
print('Generated alarm.wav and chime.wav')
