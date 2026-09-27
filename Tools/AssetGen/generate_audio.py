"""Synthesises every sound and music loop the game uses into SourceAssets/Audio (16-bit WAV),
plus audio_manifest.json for the Unreal import script.

Effects are modelled acoustically rather than as chiptune: gunshots are a crack, a muzzle blast
with a collapsing filter and a body thump played into a synthetic concrete room; zombie voices are
a glottal pulse source through vowel formant filters with jitter, vocal fry and breath noise.
Pickups/UI keep their deliberately retro blips. Music is a small step sequencer (drums, bass,
arpeggio, pads). The two sector-music layers share tempo
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


# --- acoustic building blocks ----------------------------------------------------------------------
# The effects used to be raw oscillators and bit-crushed noise, which read as 8-bit chiptune. These
# model the real thing instead: a room to hear it in, a vocal tract, a time-varying filter.

def room_reverb(x, rng, seconds=0.6, wet=0.25, damping=4000.0):
    """Convolves with a synthetic impulse response of an empty concrete room: a few discrete early
    reflections off nearby walls, then a dense exponentially decaying tail that darkens as it goes."""
    n = int(seconds * SFX_RATE)
    t = np.arange(n) / SFX_RATE
    tail = rng.standard_normal(n) * np.exp(-t * 6.9 / seconds)
    tail = lowpass(tail, damping) * 0.6 + lowpass(tail, damping * 0.3) * 0.4
    ir = tail * 0.35
    for delay, gain in ((0.009, 0.55), (0.017, 0.4), (0.026, 0.3), (0.041, 0.2)):
        ir[int(delay * SFX_RATE)] += gain
    ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
    reverberant = signal.fftconvolve(x, ir)
    dry = pad_to(x, len(reverberant))
    out = dry + wet * reverberant
    return out


def fade_tail(x, seconds=0.05):
    n = min(len(x), int(seconds * SFX_RATE))
    out = np.copy(x)
    out[-n:] *= np.linspace(1, 0, n)
    return out


def sweeping_lowpass(x, start_hz, end_hz, time_constant):
    """A lowpass whose cutoff glides from start_hz to end_hz: approximated by crossfading a bank of
    fixed filters, which is plenty for a burst of noise."""
    t = np.arange(len(x)) / SFX_RATE
    cutoff = end_hz + (start_hz - end_hz) * np.exp(-t / time_constant)
    bank = np.geomspace(end_hz, start_hz, 6)
    filtered = [lowpass(x, f) for f in bank]
    out = np.zeros(len(x))
    position = np.interp(np.log(cutoff), np.log(bank), np.arange(len(bank)))
    for i in range(len(bank)):
        weight = np.clip(1 - np.abs(position - i), 0, 1)
        out += filtered[i] * weight
    return out


def smooth_noise(n, rng, rate_hz):
    """Band-limited random wander in roughly [-1, 1], for natural pitch and loudness drift."""
    points = max(4, int(n / SFX_RATE * rate_hz) + 2)
    coarse = rng.standard_normal(points)
    return np.interp(np.linspace(0, points - 1, n), np.arange(points), coarse)


def glottal_source(rng, f0, jitter=0.015, shimmer=0.12, fry=0.0):
    """A voice's buzz: a Rosenberg glottal pulse per cycle (differentiated, as the airflow is),
    with cycle-to-cycle pitch jitter and loudness shimmer. fry > 0 makes alternate cycles weaker -
    the subharmonic crackle of a rotten, strained throat."""
    n = len(f0)
    drift = 1 + jitter * smooth_noise(n, rng, 40)
    phase = np.cumsum(f0 * drift) / SFX_RATE
    frac = phase % 1.0
    opening, closing = 0.4, 0.62
    pulse = np.where(frac < opening, 0.5 * (1 - np.cos(np.pi * frac / opening)),
                     np.where(frac < closing, np.cos(0.5 * np.pi * (frac - opening) / (closing - opening)), 0.0))
    flow = np.diff(pulse, prepend=0.0)
    cycle = np.floor(phase).astype(int)
    amps = np.clip(1 + shimmer * rng.standard_normal(cycle.max() + 2), 0.2, 2.0)
    if fry > 0:
        amps[::2] *= 1 - fry
    return flow * amps[cycle] * 60


def vowel_filter(x, formants):
    """Parallel formant resonators - (centre Hz, bandwidth Hz, gain) - the shape of the mouth."""
    out = np.zeros(len(x))
    for centre, bandwidth, gain in formants:
        b, a = signal.iirpeak(centre, centre / bandwidth, SFX_RATE)
        out += signal.lfilter(b, a, x) * gain
    return out


VOWELS = {
    "uh": ((520, 90, 1.0), (1150, 110, 0.5), (2400, 170, 0.18)),
    "oh": ((450, 80, 1.0), (800, 100, 0.6), (2500, 170, 0.12)),
    "aw": ((620, 100, 1.0), (1000, 110, 0.55), (2500, 180, 0.15)),
    "ah": ((750, 120, 1.0), (1250, 130, 0.5), (2600, 180, 0.2)),
    "eh": ((580, 100, 1.0), (1750, 140, 0.45), (2600, 180, 0.2)),
    "ee": ((300, 70, 1.0), (2200, 160, 0.35), (3000, 200, 0.15)),
}


def voice(rng, seconds, f0_start, f0_end, vowel_from, vowel_to, breath=0.15, fry=0.2, attack=0.12, release=0.5,
          growl=0.0, flutter=0.2, tract=1.0):
    """One zombie vocalisation. Pitch glides f0_start -> f0_end with a slow wander, the mouth morphs
    between two vowels, breath noise rides on top, and growl adds a fast rough amplitude modulation
    (a wet rattle in the throat). tract < 1 lowers every formant: a bigger, deeper body."""
    n = int(seconds * SFX_RATE)
    t = np.arange(n) / SFX_RATE
    contour = f0_start * (f0_end / f0_start) ** (t / seconds)
    f0 = contour * (1 + 0.04 * smooth_noise(n, rng, 3))
    source = glottal_source(rng, f0, jitter=0.02 + 0.03 * fry, shimmer=0.1 + 0.2 * fry, fry=fry)
    source += breath * bandpass(noise(n, rng), 400, 5000) * 3

    scale = lambda formants: tuple((c * tract, bw * tract, g) for c, bw, g in formants)
    start = vowel_filter(source, scale(VOWELS[vowel_from]))
    end = vowel_filter(source, scale(VOWELS[vowel_to]))
    morph = np.clip(t / seconds, 0, 1) ** 1.3
    x = start * (1 - morph) + end * morph

    if growl > 0:
        rattle = 0.5 + 0.5 * np.sin(2 * np.pi * np.cumsum(28 + 10 * smooth_noise(n, rng, 6)) / SFX_RATE)
        x *= 1 - growl + growl * rattle
    x *= np.clip(1 + flutter * smooth_noise(n, rng, 7), 0.2, 2)

    shape = np.clip(t / attack, 0, 1) ** 0.7 * np.clip((seconds - t) / release, 0, 1) ** 1.5
    x = highpass(lowpass(x * shape, 4200), 70)
    return normalise(drive(x / (np.max(np.abs(x)) + 1e-9), 1.4))


def vocal(rng, base, seconds, rough=0.4, formants=None, wobble=6.0, rise=0.0):
    """Short human-ish vocalisation used for the player and spell casts - kept for its call sites."""
    return voice(rng, seconds, base, base * (1 + rise), "uh", "ah" if rise >= 0 else "oh", breath=0.1 + 0.2 * rough,
                 fry=0.1 + 0.2 * rough, attack=min(0.05, seconds * 0.2), release=seconds * 0.6, flutter=0.1)


# --- weapons -------------------------------------------------------------------------------------

def gunshot(rng, body_hz, crack, length, bass=0.5, room=0.5, action_delay=None):
    """A firearm in a concrete building: a sharp broadband crack, a muzzle blast whose brightness
    collapses within milliseconds, a low body thump, optionally the mechanical clack of the action
    cycling, all played into a reverberant room. No bit-crushing - that is what made them sound 8-bit."""
    n = int(length * SFX_RATE)
    t = np.arange(n) / SFX_RATE

    k = int(0.0012 * SFX_RATE)
    crack_burst = np.zeros(n)
    crack_burst[:k] = rng.uniform(-1, 1, k) * np.linspace(1, 0.2, k)
    crack_burst = highpass(crack_burst, 1500)

    blast = sweeping_lowpass(noise(n, rng), crack, crack * 0.12, 0.018) * np.exp(-t / (length * 0.09))
    thump = sweep(body_hz * 2.8, body_hz * 0.75, length) * np.exp(-t / 0.05) * bass
    x = crack_burst * 1.4 + blast + thump * 1.2

    if action_delay:
        at = int(action_delay * SFX_RATE)
        m = int(0.03 * SFX_RATE)
        ping = (np.sin(2 * np.pi * 3100 * np.arange(m) / SFX_RATE) * 0.5 + bandpass(noise(m, rng), 2000, 6000))
        x[at:at + m] += ping[:max(0, min(m, n - at))] * np.exp(-np.arange(m) / (0.006 * SFX_RATE))[:max(0, min(m, n - at))] * 0.12

    x = drive(x, 1.8)
    x = room_reverb(x, rng, seconds=0.35 + room * 0.6, wet=0.2 + room * 0.25, damping=3000)
    x = pad_to(x, int((length + 0.25 + room * 0.4) * SFX_RATE))
    return normalise(fade_tail(lowpass(highpass(x, 45), 9000)))


def weapons():
    # body Hz, blast brightness, length, bass, room size, action (bolt/slide) delay
    specs = {
        "Pistol": (150, 7000, 0.3, 0.6, 0.45, 0.07), "Revolver": (105, 6000, 0.45, 0.9, 0.6, None),
        "SMG": (170, 7500, 0.22, 0.45, 0.35, 0.045), "Shotgun": (75, 4200, 0.6, 1.0, 0.75, None),
        "Rifle": (125, 8000, 0.35, 0.75, 0.6, 0.06), "Sniper": (85, 9000, 0.7, 0.95, 0.9, None),
    }
    for name, (body, crack, length, bass, room, action) in specs.items():
        for v in range(3):
            rng = np.random.default_rng(zlib.crc32(name.encode()) % 1000 + v)
            save("Weapons", "SFX_%s_Fire_%d" % (name, v), gunshot(rng, body * (1 + 0.04 * (v - 1)), crack * (1 + 0.05 * v), length, bass, room, action))

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

def gore(rng, seconds):
    """A wet impact: dull low thud plus a few squelchy filtered-noise bursts."""
    n = int(seconds * SFX_RATE)
    t = np.arange(n) / SFX_RATE
    x = lowpass(noise(n, rng), 500) * np.exp(-t / 0.08) * 1.5 + sweep(110, 45, seconds) * np.exp(-t / 0.06)
    for _ in range(3):
        at = int(rng.uniform(0.0, seconds * 0.5) * SFX_RATE)
        m = int(0.07 * SFX_RATE)
        squelch = bandpass(noise(m, rng), 250, 1400) * np.exp(-np.arange(m) / (0.02 * SFX_RATE))
        x[at:at + m] += squelch[:max(0, min(m, n - at))] * 0.6
    return x


def creatures():
    # Every group is a real vocal tract doing something different, with per-variant differences in
    # pitch, vowel and length so a horde never repeats itself. Low-passed and reverberant: these sit
    # in the mix under the gunfire rather than cutting through it like the old sawtooth moans.
    groups = {
        # idle: a low, slow, tired moan
        "Groan": lambda rng, v: voice(rng, 1.4 + 0.2 * v, 88 - 5 * v, 70 - 4 * v, "uh", ("oh", "aw", "uh", "oh")[v],
                                      breath=0.18, fry=0.35, attack=0.25, release=0.7, growl=0.15),
        # alert (spotted you): a strained, rising then breaking howl - raw, not a whistle
        "Shriek": lambda rng, v: voice(rng, 0.85 + 0.08 * v, 230 + 18 * v, 170 + 10 * v, "ah", ("eh", "aw", "ah", "eh")[v],
                                       breath=0.4, fry=0.25, attack=0.05, release=0.35, growl=0.3, flutter=0.3),
        # tank / heavy: a deep chesty roar
        "Roar": lambda rng, v: voice(rng, 1.5 + 0.1 * v, 72 - 4 * v, 55 - 3 * v, "aw", "ah", breath=0.3, fry=0.55,
                                     attack=0.12, release=0.6, growl=0.45, tract=0.8),
        # poison: a choking, bubbling gargle
        "Gurgle": lambda rng, v: voice(rng, 1.0 + 0.1 * v, 105 + 6 * v, 85, "oh", "uh", breath=0.25, fry=0.4,
                                       attack=0.1, release=0.5, growl=0.75, flutter=0.35),
        # necromancer: an airy, half-voiced hiss
        "Whisper": lambda rng, v: voice(rng, 1.3, 140 + 10 * v, 120, "ee", "ah", breath=1.4, fry=0.1,
                                        attack=0.3, release=0.7, growl=0.0, flutter=0.4),
    }
    for group, make in groups.items():
        for v in range(4):
            rng = np.random.default_rng(zlib.crc32(group.encode()) % 997 + v)
            x = make(rng, v)
            save("Zombies", "SFX_Zombie_%s_%d" % (group, v), normalise(fade_tail(room_reverb(x, rng, 0.5, 0.18)), 0.8))

    for v in range(3):
        rng = np.random.default_rng(300 + v)
        hurt = voice(rng, 0.32, 150 + 15 * v, 110, "ah", "uh", breath=0.3, fry=0.3, attack=0.01, release=0.2, growl=0.3)
        save("Zombies", "SFX_Zombie_Hurt_%d" % v, normalise(fade_tail(room_reverb(hurt, rng, 0.4, 0.15)), 0.7))

        death_voice = voice(rng, 0.9, 120 - 10 * v, 60, "aw", "uh", breath=0.3, fry=0.6, attack=0.02, release=0.6, growl=0.4)
        death = pad_to(death_voice, len(death_voice)) * 0.7 + pad_to(gore(rng, 0.5), len(death_voice))
        save("Zombies", "SFX_Zombie_Death_%d" % v, normalise(fade_tail(room_reverb(drive(death, 1.5), rng, 0.45, 0.2)), 0.8))

        # a heavy arm cutting the air: a whoosh whose pitch sweeps up then down
        n = int(0.32 * SFX_RATE)
        t = np.arange(n) / SFX_RATE
        air = noise(n, rng)
        whoosh = bandpass(air, 300, 900) * (1 - t / 0.32) + bandpass(air, 900, 2600) * np.sin(np.pi * t / 0.32)
        whoosh *= np.sin(np.pi * np.clip(t / 0.3, 0, 1)) ** 2
        save("Zombies", "SFX_Zombie_Swipe_%d" % v, normalise(lowpass(whoosh, 3500), 0.55))

    for v in range(2):
        rng = np.random.default_rng(400 + v)
        body = voice(rng, 2.4, 62 + 5 * v, 44, "aw", "ah", breath=0.35, fry=0.6, attack=0.2, release=1.0, growl=0.5, tract=0.7)
        layer = voice(rng, 2.4, 124 + 7 * v, 90, "ah", "aw", breath=0.3, fry=0.4, attack=0.3, release=1.0, growl=0.4, tract=0.75)
        n = len(body)
        sub = sweep(58, 34, 2.4) * env(n, 0.3, 2.2, curve=2)
        roar = drive(body + 0.5 * layer + 0.5 * sub, 1.6)
        save("Zombies", "SFX_Boss_Roar_%d" % v, normalise(fade_tail(room_reverb(roar, rng, 1.1, 0.3, 2500))))

    for v in range(2):
        rng = np.random.default_rng(500 + v)
        n = int(0.6 * SFX_RATE)
        splash = gore(rng, 0.6) * 0.6
        for k in range(5):
            at = int((0.08 + 0.09 * k + 0.02 * v) * SFX_RATE)
            m = int(0.05 * SFX_RATE)
            bubble = np.sin(2 * np.pi * np.cumsum(np.linspace(350 + 60 * k, 700 + 80 * k, m)) / SFX_RATE)
            splash[at:at + m] += bubble * np.exp(-np.arange(m) / (0.012 * SFX_RATE)) * 0.25
        save("Zombies", "SFX_Acid_Splash_%d" % v, normalise(lowpass(splash, 3000), 0.6))

        chant = voice(rng, 0.9, 110, 150, "oh", "ee", breath=1.0, fry=0.2, attack=0.3, release=0.4)
        shimmer = bandpass(noise(len(chant), rng), 2500, 7000) * env(len(chant), 0.4, 0.5, curve=2) * 0.15
        save("Zombies", "SFX_Necro_Cast_%d" % v, normalise(fade_tail(room_reverb(chant + shimmer, rng, 0.9, 0.35)), 0.7))


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
