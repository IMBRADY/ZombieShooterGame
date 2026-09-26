"""Synthesises every sound and music loop the game uses into SourceAssets/Audio (16-bit WAV),
plus audio_manifest.json for the Unreal import script.

Retro-flavoured synthesis: oscillators, noise, envelopes and simple filters for effects; a small
step sequencer (drums, bass, arpeggio, pads) for music. The two sector-music layers share tempo
and length so the audio manager can crossfade them sample-accurately by combat intensity.

    python Tools/AssetGen/generate_audio.py
"""

import json
import zlib
import math
import os
import wave

import numpy as np
from scipy import signal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "SourceAssets", "Audio")
SFX_RATE = 44100
MUSIC_RATE = 32000

manifest = {"sounds": []}


# --- building blocks -----------------------------------------------------------------------------

def t_axis(seconds, rate=SFX_RATE):
    return np.arange(int(seconds * rate)) / rate


def env(n, attack, decay, rate=SFX_RATE, curve=4.0):
    """Attack then exponential decay, as a sample array of length n."""
    t = np.arange(n) / rate
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    d = np.exp(-np.maximum(t - attack, 0) * curve / max(decay, 1e-4))
    return a * d


def noise(n, rng):
    return rng.uniform(-1, 1, n)


def lowpass(x, cutoff, rate=SFX_RATE, order=2):
    b, a = signal.butter(order, min(cutoff / (rate / 2), 0.99), "low")
    return signal.lfilter(b, a, x)


def highpass(x, cutoff, rate=SFX_RATE, order=2):
    b, a = signal.butter(order, min(cutoff / (rate / 2), 0.99), "high")
    return signal.lfilter(b, a, x)


def bandpass(x, lo, hi, rate=SFX_RATE):
    b, a = signal.butter(2, [lo / (rate / 2), min(hi / (rate / 2), 0.99)], "band")
    return signal.lfilter(b, a, x)


def sweep(f0, f1, seconds, rate=SFX_RATE, shape="sine"):
    t = t_axis(seconds, rate)
    freq = f0 * (f1 / f0) ** (t / max(seconds, 1e-4))
    phase = 2 * np.pi * np.cumsum(freq) / rate
    if shape == "square":
        return np.sign(np.sin(phase))
    if shape == "saw":
        return 2 * ((phase / (2 * np.pi)) % 1.0) - 1
    return np.sin(phase)


def tone(freq, seconds, rate=SFX_RATE, shape="sine"):
    return sweep(freq, freq, seconds, rate, shape)


def crush(x, bits=6):
    steps = 2 ** bits
    return np.round(x * steps) / steps


def drive(x, amount=2.5):
    return np.tanh(x * amount) / np.tanh(amount)


def echo(x, delay, feedback, rate=SFX_RATE, taps=4):
    out = np.copy(x)
    d = int(delay * rate)
    for k in range(1, taps + 1):
        if d * k < len(x):
            out[d * k:] += x[:-d * k] * (feedback ** k)
    return out


def pad_to(x, n):
    return np.pad(x, (0, max(0, n - len(x))))[:n]


def normalise(x, peak=0.9):
    m = np.max(np.abs(x)) + 1e-9
    return x / m * peak


def save(folder, name, x, rate=SFX_RATE, loop=False, category="Effects"):
    os.makedirs(os.path.join(OUT, folder), exist_ok=True)
    path = os.path.join(OUT, folder, name + ".wav")
    data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with wave.open(path, "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(data.tobytes())
    manifest["sounds"].append({"name": name, "folder": folder, "file": os.path.relpath(path, ROOT), "loop": loop, "category": category})


# --- weapons -------------------------------------------------------------------------------------

def gunshot(rng, body_hz, crack, length, bass=0.5, bits=None):
    n = int(length * SFX_RATE)
    blast = lowpass(noise(n, rng), crack) * env(n, 0.001, length * 0.35)
    thump = sweep(body_hz * 2.2, body_hz, length) * env(n, 0.001, length * 0.3) * bass
    tail = lowpass(noise(n, rng), crack * 0.35) * env(n, 0.01, length) * 0.35
    x = drive(blast + thump + tail, 2.0)
    return normalise(crush(x, bits) if bits else x)


def weapons():
    specs = {
        "Pistol": (140, 4200, 0.28, 0.6, 7), "Revolver": (95, 3500, 0.5, 0.9, None), "SMG": (170, 5200, 0.18, 0.4, 7),
        "Shotgun": (70, 2600, 0.65, 1.0, None), "Rifle": (120, 4800, 0.32, 0.7, None), "Sniper": (80, 6000, 0.9, 0.9, None),
    }
    for name, (body, crack, length, bass, bits) in specs.items():
        for v in range(3):
            rng = np.random.default_rng(zlib.crc32(name.encode()) % 1000 + v)
            save("Weapons", "SFX_%s_Fire_%d" % (name, v), gunshot(rng, body * (1 + 0.05 * v), crack, length, bass, bits))

    for v in range(2):
        rng = np.random.default_rng(50 + v)
        n = int(0.9 * SFX_RATE)
        whoosh = bandpass(noise(n, rng), 300, 2500) * env(n, 0.05, 0.9, curve=2.5)
        thump = sweep(160, 50, 0.9) * env(n, 0.001, 0.25)
        save("Weapons", "SFX_Rocket_Fire_%d" % v, normalise(drive(whoosh * 0.8 + thump, 1.5)))

        zap = sweep(1400 - 200 * v, 300, 0.35, shape="square") * env(int(0.35 * SFX_RATE), 0.002, 0.3)
        save("Weapons", "SFX_Plasma_Fire_%d" % v, normalise(lowpass(zap, 5000) + 0.2 * sweep(2800, 900, 0.35) * env(int(0.35 * SFX_RATE), 0.001, 0.2)))

        n = int(0.4 * SFX_RATE)
        roar = bandpass(noise(n, rng), 200, 1800) * env(n, 0.01, 0.4, curve=2)
        save("Weapons", "SFX_Flame_Fire_%d" % v, normalise(drive(roar, 3)))

    for v in range(2):
        rng = np.random.default_rng(80 + v)
        n = int(0.8 * SFX_RATE)
        x = np.zeros(n)
        for at, freq in ((0.05, 1800), (0.4 + 0.05 * v, 1200), (0.62, 2400)):
            k = int(at * SFX_RATE)
            m = int(0.06 * SFX_RATE)
            click = bandpass(noise(m, rng), freq * 0.6, freq * 1.8) * env(m, 0.0005, 0.05, curve=6)
            x[k:k + m] += click
        save("Weapons", "SFX_Reload_%d" % v, normalise(x, 0.7))

    m = int(0.12 * SFX_RATE)
    rng = np.random.default_rng(99)
    save("Weapons", "SFX_Empty", normalise(bandpass(noise(m, rng), 2500, 7000) * env(m, 0.0005, 0.04, curve=8), 0.5))

    for v in range(3):
        rng = np.random.default_rng(120 + v)
        n = int(1.6 * SFX_RATE)
        boom = lowpass(noise(n, rng), 900 - v * 100) * env(n, 0.002, 1.4, curve=3)
        sub = sweep(90, 30, 1.6) * env(n, 0.001, 0.8, curve=3)
        crackle = highpass(noise(n, rng), 3000) * env(n, 0.001, 0.25) * 0.3
        save("Weapons", "SFX_Explosion_%d" % v, normalise(drive(boom + sub * 0.9 + crackle, 2.2)))


# --- creatures -----------------------------------------------------------------------------------

def vocal(rng, base, seconds, rough=0.4, formants=((500, 1100), (700, 1500)), wobble=6.0, rise=0.0):
    """A moan: a buzzy glottal source through vowel formants, with pitch wobble."""
    t = t_axis(seconds)
    pitch = base * (1 + rise * t / seconds) * (1 + 0.06 * np.sin(2 * np.pi * wobble * t) + 0.03 * rng.standard_normal(len(t)).cumsum() / 200)
    phase = 2 * np.pi * np.cumsum(pitch) / SFX_RATE
    source = 2 * ((phase / (2 * np.pi)) % 1.0) - 1 + rough * noise(len(t), rng)
    x = np.zeros(len(t))
    for lo, hi in formants:
        x += bandpass(source, lo, hi)
    shape = env(len(t), seconds * 0.25, seconds * 0.9, curve=2.0)
    return normalise(drive(x * shape, 1.8))


def creatures():
    groups = {
        "Groan": dict(base=85, seconds=1.3, rough=0.35),
        "Shriek": dict(base=260, seconds=0.7, rough=0.6, formants=((900, 2200), (1800, 3500)), rise=0.4),
        "Roar": dict(base=55, seconds=1.6, rough=0.7, formants=((250, 700), (500, 1200))),
        "Gurgle": dict(base=110, seconds=1.0, rough=0.9, formants=((300, 900),), wobble=14),
        "Whisper": dict(base=180, seconds=1.4, rough=1.6, formants=((1200, 3200),), wobble=3),
    }
    for group, params in groups.items():
        for v in range(4):
            rng = np.random.default_rng(zlib.crc32(group.encode()) % 997 + v)
            p = dict(params)
            p["base"] = p["base"] * (0.9 + 0.07 * v)
            save("Zombies", "SFX_Zombie_%s_%d" % (group, v), vocal(rng, **p))

    for v in range(3):
        rng = np.random.default_rng(300 + v)
        save("Zombies", "SFX_Zombie_Hurt_%d" % v, vocal(rng, 120 + 20 * v, 0.35, rough=0.8))
        n = int(0.5 * SFX_RATE)
        splat = lowpass(noise(n, rng), 1200) * env(n, 0.002, 0.35, curve=5)
        save("Zombies", "SFX_Zombie_Death_%d" % v, normalise(drive(splat + 0.5 * vocal(rng, 90, 0.5, rough=0.9)[:n], 2)))
        n = int(0.3 * SFX_RATE)
        swipe = bandpass(noise(n, rng), 800, 4000) * env(n, 0.05, 0.25, curve=3)
        save("Zombies", "SFX_Zombie_Swipe_%d" % v, normalise(swipe, 0.6))

    for v in range(2):
        rng = np.random.default_rng(400 + v)
        roar = vocal(rng, 45 + 6 * v, 2.4, rough=0.9, formants=((150, 500), (400, 1000), (900, 1800)))
        save("Zombies", "SFX_Boss_Roar_%d" % v, normalise(drive(roar + 0.4 * sweep(60, 35, 2.4) * env(len(roar), 0.3, 2.2, curve=2), 2)))

    for v in range(2):
        rng = np.random.default_rng(500 + v)
        n = int(0.6 * SFX_RATE)
        bubble = sum(tone(300 + 180 * k + 40 * v, 0.6) * env(n, 0.01 + 0.12 * k, 0.08) for k in range(4))
        save("Zombies", "SFX_Acid_Splash_%d" % v, normalise(lowpass(bubble + 0.3 * noise(n, rng) * env(n, 0.001, 0.2), 2500), 0.6))
        save("Zombies", "SFX_Necro_Cast_%d" % v, normalise(sweep(200, 900, 0.9, shape="saw") * env(int(0.9 * SFX_RATE), 0.3, 0.9) * 0.5
                                                          + vocal(rng, 150, 0.9, rough=1.2, formants=((600, 2000),))[:int(0.9 * SFX_RATE)]))


# --- player, pickups and UI ------------------------------------------------------------------------

def blip(freqs, step, shape="square", decay=0.12):
    parts = [tone(f, step, shape=shape) * env(int(step * SFX_RATE), 0.002, decay) for f in freqs]
    return normalise(lowpass(np.concatenate(parts), 6000), 0.55)


def interface():
    rng = np.random.default_rng(600)
    save("Player", "SFX_Player_Hurt", vocal(rng, 150, 0.35, rough=0.5, formants=((600, 1400),)))
    save("Player", "SFX_Player_Death", vocal(rng, 110, 1.4, rough=0.6, formants=((500, 1200),), rise=-0.5))
    n = int(0.12 * SFX_RATE)
    save("Player", "SFX_Footstep", normalise(lowpass(noise(n, rng), 600) * env(n, 0.002, 0.08, curve=6), 0.4))

    save("Pickups", "SFX_Pickup_Coin", blip([988, 1319], 0.07))
    save("Pickups", "SFX_Pickup_Health", blip([523, 659, 784], 0.08, "sine"))
    save("Pickups", "SFX_Pickup_Armor", blip([392, 523, 659], 0.08, "square"))
    save("Pickups", "SFX_Pickup_Ammo", blip([330, 440], 0.06))
    save("Pickups", "SFX_Pickup_Weapon", blip([440, 554, 659, 880], 0.07))
    save("Pickups", "SFX_Pickup_Perk", blip([523, 659, 784, 1047, 1319], 0.07, "sine", 0.2))
    save("Pickups", "SFX_Pickup_Key", blip([784, 988, 1175, 1568], 0.09, "square", 0.25))

    save("UI", "SFX_UI_Click", blip([1200], 0.04, "square", 0.03), category="UI")
    save("UI", "SFX_UI_Buy", blip([659, 988, 1319], 0.06, "square", 0.1), category="UI")
    save("UI", "SFX_UI_Denied", blip([220, 175], 0.1, "square", 0.12), category="UI")
    save("UI", "SFX_Achievement", blip([523, 659, 784, 1047, 784, 1047], 0.09, "square", 0.3), category="UI")
    save("UI", "SFX_Sector_Cleared", blip([392, 523, 659, 784, 1047], 0.12, "square", 0.3), category="UI")

    n = int(0.9 * SFX_RATE)
    clank = bandpass(noise(n, rng), 400, 3000) * env(n, 0.001, 0.3, curve=4) + tone(180, 0.9) * env(n, 0.001, 0.5) * 0.5
    save("World", "SFX_Door_Unlock", normalise(np.concatenate([blip([523, 784], 0.08), clank * 0.7]), 0.7))
    motor = sweep(60, 90, 1.2, shape="saw") * env(int(1.2 * SFX_RATE), 0.1, 1.1, curve=1.5)
    save("World", "SFX_Door_Open", normalise(lowpass(motor, 800) + 0.2 * noise(len(motor), rng) * env(len(motor), 0.1, 1.0, curve=2), 0.7))
    creak = sweep(300, 180, 0.6, shape="saw") * env(int(0.6 * SFX_RATE), 0.05, 0.6)
    save("World", "SFX_Chest_Open", normalise(bandpass(creak, 200, 1500), 0.6))


# --- music ---------------------------------------------------------------------------------------

NOTE = {n: i for i, n in enumerate(["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"])}


def midi(note, octave):
    return 440.0 * 2 ** ((NOTE[note] + 12 * (octave + 1) - 69) / 12)


class Track:
    def __init__(self, bpm, bars, rate=MUSIC_RATE):
        self.rate = rate
        self.step = 60.0 / bpm / 4
        self.steps = bars * 16
        self.buf = np.zeros(int(self.steps * self.step * rate) + rate)

    def add(self, at_step, x, gain=1.0):
        i = int(at_step * self.step * self.rate)
        n = min(len(x), len(self.buf) - i)
        if n > 0:
            self.buf[i:i + n] += x[:n] * gain

    def note(self, freq, length_steps, shape="saw", attack=0.005, decay=None, cutoff=None):
        seconds = length_steps * self.step
        t = np.arange(int(seconds * self.rate)) / self.rate
        phase = 2 * np.pi * freq * t
        if shape == "saw":
            x = 2 * ((freq * t) % 1.0) - 1
        elif shape == "square":
            x = np.sign(np.sin(phase)) * 0.7
        elif shape == "tri":
            x = 2 * np.abs(2 * ((freq * t) % 1.0) - 1) - 1
        else:
            x = np.sin(phase)
        e = env(len(t), attack, decay or seconds, self.rate, curve=3.0)
        x = x * e
        return lowpass(x, cutoff, self.rate) if cutoff else x

    def kick(self):
        n = int(0.25 * self.rate)
        return sweep(150, 42, 0.25, self.rate) * env(n, 0.001, 0.2, self.rate, curve=4)

    def snare(self, rng):
        n = int(0.2 * self.rate)
        return (bandpass(noise(n, rng), 800, 6000, self.rate) * 0.8 + tone(190, 0.2, self.rate) * 0.3) * env(n, 0.001, 0.15, self.rate, curve=5)

    def hat(self, rng, length=0.05):
        n = int(length * self.rate)
        return highpass(noise(n, rng), 7000, self.rate) * env(n, 0.0005, length, self.rate, curve=6)

    def render(self):
        length = int(self.steps * self.step * self.rate)
        out = self.buf[:length].copy()
        # Fold the tail that ran past the loop point back to the start, so the loop is seamless.
        tail = self.buf[length:]
        out[:len(tail)] += tail
        return out


PROGRESSION = [("A", "C", "E"), ("F", "A", "C"), ("C", "E", "G"), ("G", "B", "D")]
ROOTS = ["A", "F", "C", "G"]


def sector_layers():
    bpm, bars = 132, 8
    rng = np.random.default_rng(7)

    explore = Track(bpm, bars)
    for bar in range(bars):
        root = ROOTS[bar % 4]
        chord = PROGRESSION[bar % 4]
        base = bar * 16
        explore.add(base, explore.note(midi(root, 1), 16, "saw", attack=0.4, cutoff=500), 0.35)
        for k, n in enumerate(chord):
            explore.add(base, explore.note(midi(n, 3), 16, "tri", attack=0.8), 0.08)
        for s in range(0, 16, 4):
            explore.add(base + s, explore.note(midi(root, 2), 2, "square", decay=0.15, cutoff=900), 0.18)
        explore.add(base, explore.kick(), 0.35)
        explore.add(base + 8, explore.kick(), 0.25)
        for s in range(2, 16, 4):
            explore.add(base + s, explore.hat(rng), 0.08)

    combat = Track(bpm, bars)
    arp_pattern = [0, 1, 2, 1, 0, 2, 1, 2]
    for bar in range(bars):
        root = ROOTS[bar % 4]
        chord = PROGRESSION[bar % 4]
        base = bar * 16
        for s in range(16):
            combat.add(base + s, combat.note(midi(root, 1 if s % 2 == 0 else 2), 1, "saw", decay=0.12, cutoff=1200), 0.3)
            combat.add(base + s, combat.hat(rng, 0.03), 0.07 if s % 2 else 0.1)
            if s % 2 == 0:
                n = chord[arp_pattern[(s // 2) % len(arp_pattern)]]
                combat.add(base + s, combat.note(midi(n, 4), 2, "square", decay=0.2, cutoff=3000), 0.1)
        for s in (0, 6, 8, 11):
            combat.add(base + s, combat.kick(), 0.5)
        for s in (4, 12):
            combat.add(base + s, combat.snare(rng), 0.4)
    return explore.render(), combat.render()


def simple_theme(bpm, bars, roots, chords, drums, lead_octave, seed, shape="square", pad_gain=0.08, bass_gain=0.3):
    rng = np.random.default_rng(seed)
    track = Track(bpm, bars)
    for bar in range(bars):
        root, chord, base = roots[bar % len(roots)], chords[bar % len(chords)], bar * 16
        track.add(base, track.note(midi(root, 1), 16, "saw", attack=0.2, cutoff=600), bass_gain)
        for n in chord:
            track.add(base, track.note(midi(n, 3), 16, "tri", attack=0.5), pad_gain)
        if lead_octave:
            for s in range(0, 16, 4 if bar % 2 else 2):
                n = chord[(s // 2 + bar) % 3]
                track.add(base + s, track.note(midi(n, lead_octave), 2, shape, decay=0.25, cutoff=2500), 0.09)
        if drums == "heavy":
            for s in (0, 3, 8, 10):
                track.add(base + s, track.kick(), 0.55)
            for s in (4, 12):
                track.add(base + s, track.snare(rng), 0.45)
            for s in range(0, 16, 2):
                track.add(base + s, track.hat(rng), 0.08)
        elif drums == "soft":
            track.add(base, track.kick(), 0.25)
            for s in (6, 14):
                track.add(base + s, track.hat(rng, 0.08), 0.06)
    return track.render()


def music():
    explore, combat = sector_layers()
    save("Music", "MUS_Sector_Explore", normalise(explore, 0.8), MUSIC_RATE, loop=True, category="Music")
    save("Music", "MUS_Sector_Combat", normalise(combat, 0.8), MUSIC_RATE, loop=True, category="Music")

    minor = [("D", "F", "A"), ("A#", "D", "F"), ("G", "A#", "D"), ("A", "C#", "E")]
    save("Music", "MUS_Menu", normalise(simple_theme(80, 8, ["D", "A#", "G", "A"], minor, "soft", 4, 11, "tri", 0.1), 0.75),
         MUSIC_RATE, loop=True, category="Music")
    save("Music", "MUS_Boss", normalise(simple_theme(150, 8, ["E", "C", "D", "B"], [("E", "G", "B"), ("C", "E", "G"), ("D", "F#", "A"), ("B", "D#", "F#")],
                                                     "heavy", 5, 13, "saw", 0.06, 0.4), 0.8), MUSIC_RATE, loop=True, category="Music")
    save("Music", "MUS_Intermission", normalise(simple_theme(96, 8, ["C", "A", "F", "G"], [("C", "E", "G"), ("A", "C", "E"), ("F", "A", "C"), ("G", "B", "D")],
                                                             "soft", 5, 17, "tri", 0.07, 0.2), 0.7), MUSIC_RATE, loop=True, category="Music")
    save("Music", "MUS_GameOver", normalise(simple_theme(60, 4, ["A", "F", "D", "E"], [("A", "C", "E"), ("F", "A", "C"), ("D", "F", "A"), ("E", "G#", "B")],
                                                         None, 0, 19, "tri", 0.12, 0.25), 0.7), MUSIC_RATE, loop=True, category="Music")

    rng = np.random.default_rng(23)
    seconds = 12.0
    n = int(seconds * MUSIC_RATE)
    t = np.arange(n) / MUSIC_RATE
    hum = 0.4 * np.sin(2 * np.pi * 50 * t) + 0.2 * np.sin(2 * np.pi * 100 * t) + 0.1 * np.sin(2 * np.pi * 150.5 * t)
    rumble = lowpass(noise(n, rng), 200, MUSIC_RATE) * 1.5
    clanks = np.zeros(n)
    for at in (1.3, 4.1, 7.7, 10.2):
        k = int(at * MUSIC_RATE)
        m = int(0.4 * MUSIC_RATE)
        clanks[k:k + m] += bandpass(noise(m, rng), 300, 1800, MUSIC_RATE) * env(m, 0.001, 0.3, MUSIC_RATE, curve=5) * 0.4
    ambience = hum * (0.8 + 0.2 * np.sin(2 * np.pi * t / seconds)) + rumble + clanks
    save("Ambience", "AMB_Machinery", normalise(ambience, 0.6), MUSIC_RATE, loop=True, category="Effects")


def main():
    weapons()
    creatures()
    interface()
    music()
    with open(os.path.join(OUT, "audio_manifest.json"), "w") as handle:
        json.dump(manifest, handle, indent=1)
    print("Generated %d sounds into %s" % (len(manifest["sounds"]), OUT))


if __name__ == "__main__":
    main()
