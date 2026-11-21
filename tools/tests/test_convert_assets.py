"""Tests for tools\\assets\\convert_assets.py's pure rules: reading a game .wav's chunks, the Vorbis comments, argon --decode's
report, the fact cut and the Vorbis bit rate search (no ffmpeg: a fake encoder check).

run: python -m unittest discover -s tools\\tests   (test.bat runs it through CTest)
"""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'assets'))
import convert_assets

PCM_SAMPLES = bytes(range(16))
TRAILING_TAGS = b"INFOISFT\x05\x00\x00\x00tags\x00\x00"


def chunk(chunk_id, body):
    return struct.pack("<4sI", chunk_id, len(body)) + body + (b"\x00" if len(body) & 1 else b"")


def riff(*chunks):
    body = b"WAVE" + b"".join(chunks)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def pcm_format():
    return chunk(b"fmt ", struct.pack("<HHIIHH", 1, 2, 44100, 176400, 4, 16))


def adpcm_format():
    return chunk(b"fmt ", struct.pack("<HHIIHH", 0x69, 1, 22050, 12403, 36, 4))


def sampler(loops):
    header = struct.pack("<9I", 0, 0, 0, 60, 0, 0, 0, len(loops), 0)
    return chunk(b"smpl", header + b"".join(struct.pack("<6I", 0, 0, start, end, 0, 0) for start, end in loops))


class FakeVorbisLimits(convert_assets.Converter):
    """accepts the bit rates in [lowest, highest] kbit/s"""

    def __init__(self, lowest, highest):
        super().__init__(Path("ffmpeg"), Path("argon"), False)
        self.lowest = lowest
        self.highest = highest

    def vorbis_accepts(self, rate, channels, kbits):
        return self.lowest <= kbits <= self.highest


class ParseWavTests(unittest.TestCase):
    def test_data_stops_before_the_trailing_list_chunk(self):
        # Arrange
        contents = riff(pcm_format(), chunk(b"data", PCM_SAMPLES), chunk(b"LIST", TRAILING_TAGS))

        # Act
        wav = convert_assets.parse_wav(contents)

        # Assert
        self.assertEqual(wav.data, PCM_SAMPLES)

    def test_reads_the_fact_frame_count(self):
        # Arrange
        contents = riff(adpcm_format(), chunk(b"fact", struct.pack("<I", 1234)), chunk(b"data", bytes(36)))

        # Act
        wav = convert_assets.parse_wav(contents)

        # Assert
        self.assertEqual(wav.fact_frames, 1234)

    def test_reads_the_first_sampler_loop(self):
        # Arrange
        contents = riff(adpcm_format(), chunk(b"data", bytes(36)), sampler([(10, 99), (5, 6)]))

        # Act
        wav = convert_assets.parse_wav(contents)

        # Assert
        self.assertEqual((wav.loop_start, wav.loop_end), (10, 99))

    def test_sampler_without_loops_is_no_loop(self):
        # Arrange
        contents = riff(pcm_format(), chunk(b"data", PCM_SAMPLES), sampler([]))

        # Act
        wav = convert_assets.parse_wav(contents)

        # Assert
        self.assertEqual((wav.loop_start, wav.loop_end), (0, 0))

    def test_rejects_a_file_that_is_not_riff_wave(self):
        # Arrange / Act / Assert
        with self.assertRaises(ValueError):
            convert_assets.parse_wav(b"OggS" + bytes(40))


class EffectCommentTests(unittest.TestCase):
    def test_inclusive_end_becomes_a_length(self):
        # Arrange
        wav = convert_assets.WavFile(loop_start=100, loop_end=399)

        # Act
        comments = convert_assets.effect_comments(wav)

        # Assert
        self.assertEqual(comments, {"LOOPSTART": 100, "LOOPLENGTH": 300})

    def test_fact_count_becomes_the_play_length(self):
        # Arrange
        wav = convert_assets.WavFile(fact_frames=6250)

        # Act
        comments = convert_assets.effect_comments(wav)

        # Assert
        self.assertEqual(comments, {"PLAYLENGTH": 6250})

    def test_no_comments_without_fact_or_loop(self):
        # Arrange / Act / Assert
        self.assertEqual(convert_assets.effect_comments(convert_assets.WavFile()), {})


class DecodeReportTests(unittest.TestCase):
    def test_reads_play_length_and_loop(self):
        # Arrange
        report = "a.ogg: 1 ch, 22050 Hz, 0.29 s, peak 32461, play 6250, loop [0, 6280)\n"

        # Act / Assert
        self.assertEqual(convert_assets.parse_decode_report(report), (6250, 0, 6280))

    def test_rejects_an_unexpected_report(self):
        # Arrange / Act / Assert
        with self.assertRaises(ValueError):
            convert_assets.parse_decode_report("cannot read a.ogg")


class FirstFramesTests(unittest.TestCase):
    def test_cuts_to_whole_frames_of_every_channel(self):
        # Arrange
        pcm = convert_assets.Pcm(44100, 2, PCM_SAMPLES)

        # Act
        cut = convert_assets.first_frames(pcm, 3)

        # Assert
        self.assertEqual(cut.samples, PCM_SAMPLES[:12])


class VorbisBitRateTests(unittest.TestCase):
    def test_keeps_256_kbits_when_accepted(self):
        # Arrange
        converter = FakeVorbisLimits(32, 500)

        # Act / Assert
        self.assertEqual(converter.vorbis_kbits(44100, 2), 256)

    def test_finds_the_highest_accepted_below_256(self):
        # Arrange
        converter = FakeVorbisLimits(30, 90)

        # Act / Assert
        self.assertEqual(converter.vorbis_kbits(22050, 1), 90)

    def test_fails_when_nothing_is_accepted(self):
        # Arrange
        converter = FakeVorbisLimits(1000, 2000)

        # Act / Assert
        with self.assertRaises(RuntimeError):
            converter.vorbis_kbits(8000, 1)


if __name__ == "__main__":
    unittest.main()
