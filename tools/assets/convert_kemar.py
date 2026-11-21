"""Converts the MIT KEMAR HRTF measurements (Gardner & Martin, MIT Media Lab, 1994: the "full" set, normal pinna) into
<project>/hrtf_kemar.bin, the alternative to the Xbox's own HRTF that argon plays when Argon Options' HRTF is
MIT KEMAR (audio\\kemar.cpp, docs\\audio\\mix-levels.md "Mix levels").

The full set: elev<e>/L<e>e<aaa>a.wav, 512 samples at 44.1 kHz, one per ear and direction (L the normal pinna, R the
large one). Only the L files are used: the right ear at azimuth a is the left ear at 360 - a, as in the compact set.

Per direction and ear: resampled to 48 kHz, diffuse-field equalized (the average over every direction made flat,
which takes out the loudspeaker and the ear canal), made minimum-phase and cut to 512 taps (about the measurement's
whole length: 557 samples at 48 kHz), the last 8 faded out; the onset difference between the ears becomes the far ear's delay, as the
Xbox's filters hold it. Argon cuts the taps to Argon Options' KEMAR Taps (64, 128, 256 or 512) with the same fade. The
set is scaled so the front pair cut to 64 taps (the default) passes 200 Hz..12 kHz at -7.5 dB, the level of the Xbox's
front filter #1170. It prints, per tap choice, the front pair's level and the energy the cut leaves out.

hrtf_kemar.bin (little-endian): "ARKM", u32 version 1, u32 taps, u32 ring count; per ring (elevation ascending): i32
elevation, u32 count; per direction (azimuth 0..180 ascending, + to the right): i32 azimuth, f32 delay in samples at
48 kHz (on the left ear), f32 left[taps], f32 right[taps].

Argon reads it from the project folder (beside game_data and save_data), else from beside argon.exe.

usage: python -I tools/assets/convert_kemar.py <full set folder> [--out <file>]   (needs numpy)
"""
import argparse
import re
import struct
import sys
import wave
from pathlib import Path

import numpy as np

SOURCE_RATE, RATE = 44100, 48000
TAP_CHOICES = (64, 128, 256, 512)   # Argon Options' KEMAR Taps (app_settings.cpp)
TAPS, DEFAULT_TAPS, FADE_TAPS = TAP_CHOICES[-1], TAP_CHOICES[0], 8
SPECTRUM_SIZE = 4096
EQ_LOW_HZ, EQ_HIGH_HZ, EQ_RANGE_DB = 200.0, 18000.0, 15.0
LEVEL_LOW_HZ, LEVEL_HIGH_HZ, FRONT_DB = 200.0, 12000.0, -7.5
ONSET_FRACTION, ONSET_OVERSAMPLING = 0.2, 8
MAGIC, VERSION = b"ARKM", 1
FILE_NAME = re.compile(r"^L(-?\d+)e(\d{3})a\.wav$", re.IGNORECASE)


def read_impulse(path):
    with wave.open(str(path), "rb") as file:
        if file.getframerate() != SOURCE_RATE or file.getsampwidth() != 2 or file.getnchannels() != 1:
            raise ValueError(f"{path}: not a 16-bit mono 44.1 kHz KEMAR impulse")
        samples = np.frombuffer(file.readframes(file.getnframes()), dtype="<i2")
    return samples.astype(np.float64) / 32768.0


def read_left_ears(folder):
    """{elevation: {azimuth: impulse}} of every L file under `folder`."""
    rings = {}
    for path in folder.rglob("*.wav"):
        match = FILE_NAME.match(path.name)
        if match:
            rings.setdefault(int(match.group(1)), {})[int(match.group(2))] = read_impulse(path)
    if 0 not in rings or 90 not in rings[0] or 270 not in rings[0]:
        raise ValueError(f"no MIT KEMAR full set (elev0/L0e090a.wav, L0e270a.wav ...) under {folder}")
    return rings


def to_the_right(rings):
    """The azimuths turned so 90 is to the right: the left ear hears a source at 270 louder than at 90."""
    horizon = rings[0]
    if np.sum(horizon[270] ** 2) > np.sum(horizon[90] ** 2):
        return rings
    return {elevation: {(360 - azimuth) % 360: impulse for azimuth, impulse in ring.items()} for elevation, ring in rings.items()}


def resampled(impulse):
    """Band-limited 44.1 -> 48 kHz: the spectrum of 441 k samples zero-extended to 480 k."""
    blocks = -(-len(impulse) // 441)
    spectrum = np.fft.rfft(impulse, 441 * blocks)
    out_size = 480 * blocks
    extended = np.zeros(out_size // 2 + 1, dtype=complex)
    extended[:len(spectrum)] = spectrum
    return np.fft.irfft(extended, out_size) * out_size / (441 * blocks)


def onset(impulse):
    """The first sample, in 48 kHz samples, within ONSET_FRACTION of the peak (oversampled)."""
    size = len(impulse) * ONSET_OVERSAMPLING
    spectrum = np.fft.rfft(impulse)
    fine = np.fft.irfft(np.concatenate([spectrum, np.zeros(size // 2 + 1 - len(spectrum))]), size)
    magnitude = np.abs(fine)
    return np.argmax(magnitude >= ONSET_FRACTION * magnitude.max()) / ONSET_OVERSAMPLING


def nearest(ring, azimuth):
    """The ring's impulse measured closest to `azimuth` (the rings' azimuths are rounded to whole degrees)."""
    distances = {measured: abs((measured - azimuth + 180) % 360 - 180) for measured in ring}
    return ring[min(distances, key=distances.get)]


def ear_pairs(rings):
    """{elevation: {azimuth 0..180: (left, right)}} at 48 kHz; the right ear mirrors the left."""
    pairs = {}
    for elevation, ring in sorted(rings.items()):
        pairs[elevation] = {azimuth: (resampled(ring[azimuth]), resampled(nearest(ring, 360 - azimuth)))
                            for azimuth in sorted(ring) if azimuth <= 180}
    return pairs


def frequencies():
    return np.fft.rfftfreq(SPECTRUM_SIZE, 1.0 / RATE)


def diffuse_field_eq(pairs):
    """The inverse of the average magnitude over every ear and direction, held outside EQ_LOW_HZ..EQ_HIGH_HZ and within
    EQ_RANGE_DB of its value at 1 kHz."""
    power = np.mean([np.abs(np.fft.rfft(ear, SPECTRUM_SIZE)) ** 2 for ring in pairs.values() for pair in ring.values() for ear in pair], axis=0)
    eq_db = -10.0 * np.log10(np.maximum(power, 1e-12))
    hz = frequencies()
    in_band = (hz >= EQ_LOW_HZ) & (hz <= EQ_HIGH_HZ)
    eq_db = np.interp(hz, hz[in_band], eq_db[in_band])
    reference = np.interp(1000.0, hz, eq_db)
    return 10.0 ** (np.clip(eq_db, reference - EQ_RANGE_DB, reference + EQ_RANGE_DB) / 20.0)


def minimum_phase(magnitude):
    """The minimum-phase impulse of a magnitude spectrum (the folded real cepstrum), SPECTRUM_SIZE samples long."""
    cepstrum = np.fft.irfft(np.log(np.maximum(magnitude, 1e-6)), SPECTRUM_SIZE)
    fold = np.zeros(SPECTRUM_SIZE)
    fold[0] = 1.0
    fold[1:SPECTRUM_SIZE // 2] = 2.0
    fold[SPECTRUM_SIZE // 2] = 1.0
    return np.fft.irfft(np.exp(np.fft.rfft(cepstrum * fold)), SPECTRUM_SIZE)


def cut(impulse, taps):
    """The first `taps` samples, the last FADE_TAPS faded out (cos^2), as argon cuts them (kemar.cpp)."""
    kept = impulse[:taps].copy()
    kept[-FADE_TAPS:] *= np.cos(np.linspace(0.0, np.pi / 2, FADE_TAPS + 1)[1:]) ** 2
    return kept


def band_level_db(impulse):
    hz = frequencies()
    band = (hz >= LEVEL_LOW_HZ) & (hz <= LEVEL_HIGH_HZ)
    return 10.0 * np.log10(np.mean(np.abs(np.fft.rfft(impulse, SPECTRUM_SIZE))[band] ** 2))


def full_filters(pairs):
    """{elevation: [(azimuth, delay, left, right)]}: the unscaled minimum-phase pairs, SPECTRUM_SIZE samples long."""
    eq = diffuse_field_eq(pairs)
    converted = {}
    for elevation, ring in pairs.items():
        converted[elevation] = []
        for azimuth, (left, right) in ring.items():
            delay = max(0.0, onset(left) - onset(right))
            converted[elevation].append((azimuth, delay, minimum_phase(np.abs(np.fft.rfft(left, SPECTRUM_SIZE)) * eq),
                                         minimum_phase(np.abs(np.fft.rfft(right, SPECTRUM_SIZE)) * eq)))
    return converted


def front_level_db(converted, taps):
    """The front pair's level cut to `taps`, both ears averaged."""
    front = converted[0][0]
    return (band_level_db(cut(front[2], taps)) + band_level_db(cut(front[3], taps))) / 2.0


def filters(converted):
    """{elevation: [(azimuth, delay, left TAPS, right TAPS)]}, scaled so the front pair cut to DEFAULT_TAPS sits at
    FRONT_DB."""
    scale = 10.0 ** ((FRONT_DB - front_level_db(converted, DEFAULT_TAPS)) / 20.0)
    return {elevation: [(azimuth, delay, cut(left, TAPS) * scale, cut(right, TAPS) * scale) for azimuth, delay, left, right in ring]
            for elevation, ring in converted.items()}


def tail_db(impulse, taps):
    """The energy past `taps`, in dB of the whole impulse's."""
    return 10.0 * np.log10(max(np.sum(impulse[taps:] ** 2), 1e-30) / np.sum(impulse ** 2))


def report(converted):
    """Per tap choice: the front pair's level once scaled, and the energy the cut leaves out (worst / median ear)."""
    offset = FRONT_DB - front_level_db(converted, DEFAULT_TAPS)
    ears = [ear for ring in converted.values() for _, _, left, right in ring for ear in (left, right)]
    delays = [delay for ring in converted.values() for _, delay, _, _ in ring]
    print(f"far-ear delay: up to {max(delays):.1f} samples ({max(delays) / RATE * 1000.0:.2f} ms)")
    for taps in TAP_CHOICES:
        tails = [tail_db(ear, taps) for ear in ears]
        print(f"{taps:4d} taps: front pair {front_level_db(converted, taps) + offset:+.2f} dB, "
              f"tail past the cut {max(tails):.1f} dB (worst ear), {np.median(tails):.1f} dB (median)")


def packed(converted):
    data = bytearray(MAGIC + struct.pack("<3I", VERSION, TAPS, len(converted)))
    for elevation, ring in sorted(converted.items()):
        data += struct.pack("<iI", elevation, len(ring))
        for azimuth, delay, left, right in ring:
            data += struct.pack("<if", azimuth, delay) + left.astype("<f4").tobytes() + right.astype("<f4").tobytes()
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("full_set", type=Path, help="the MIT KEMAR full set's folder (the elev* folders under it)")
    parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parents[2] / "hrtf_kemar.bin")
    args = parser.parse_args()
    try:
        rings = to_the_right(read_left_ears(args.full_set))
    except ValueError as error:
        sys.exit(str(error))
    full = full_filters(ear_pairs(rings))
    converted = filters(full)
    out_path = args.out
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(packed(converted))
    count = sum(len(ring) for ring in converted.values())
    print(f"wrote {out_path}: {count} directions on {len(converted)} elevations, {TAPS} taps")
    report(full)


if __name__ == "__main__":
    main()
