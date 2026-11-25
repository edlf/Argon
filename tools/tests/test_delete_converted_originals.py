"""Tests for tools\\assets\\delete_converted_originals.py and the discovery it shares with tools\\assets\\convert_assets.py: which
originals are found, their status (a fake verifier: no ffmpeg or argon_decode), the summary and what --delete removes.

run: python -m unittest discover -s tools\\tests   (test.bat runs it through CTest)
"""
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'assets'))
import convert_assets
import delete_converted_originals as deleter

ORIGINAL_BYTES = b"RIFF original"
CONVERTED_BYTES = b"converted"


class FakeVerifier:
    """passes every conversion, or raises the given error"""

    def __init__(self, error=None):
        self.error = error

    def verify(self, conversion):
        if self.error:
            raise self.error


class GameFolderTestCase(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.game = Path(self.folder.name)

    def tearDown(self):
        self.folder.cleanup()

    def write(self, relative, contents=ORIGINAL_BYTES, age_seconds=0):
        path = self.game / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents)
        stamp = path.stat().st_mtime - age_seconds
        os.utime(path, (stamp, stamp))
        return path

    def effect(self, converted=True, converted_age_seconds=0):
        """audio/generic/bronze/Bronze_L.wav, an hour old, and its .ogg"""
        source = self.write("audio/generic/bronze/Bronze_L.wav", age_seconds=3600)
        if converted:
            self.write("audio/generic/bronze/Bronze_L.ogg", CONVERTED_BYTES, converted_age_seconds)
        return convert_assets.Conversion(convert_assets.EFFECT_KIND, source, source.with_suffix(".ogg"))


class FindConversionsTests(GameFolderTestCase):
    def test_finds_videos_music_and_effects(self):
        # Arrange
        self.write("media/videos/curly_seq.bik")
        self.write("media/music/silver_music/on_start.wav")
        self.write("audio/generic/bronze/Bronze_L.wav")

        # Act
        kinds = [conversion.kind for conversion in convert_assets.find_conversions(self.game)]

        # Assert
        self.assertEqual(kinds, [convert_assets.VIDEO_KIND, convert_assets.MUSIC_KIND, convert_assets.EFFECT_KIND])

    def test_targets_sit_next_to_the_originals(self):
        # Arrange
        self.write("Locale/English/intro.bik")
        self.write("media/music/silver_music/on_start.wav")
        self.write("audio/generic/bronze/Bronze_L.wav")

        # Act
        targets = [conversion.target for conversion in convert_assets.find_conversions(self.game)]

        # Assert
        self.assertEqual(targets, [self.game / "Locale/English/intro.ogv", self.game / "media/music/silver_music/on_start.flac",
                                   self.game / "audio/generic/bronze/Bronze_L.ogg"])

    def test_skips_the_story_speech_and_other_files(self):
        # Arrange
        self.write("data/movies/speech/tara/tara_1.wav")
        self.write("audio/generic/bronze/bronze.txt")
        self.write("audio/generic/bronze/Bronze_L.ogg")

        # Act
        conversions = convert_assets.find_conversions(self.game)

        # Assert
        self.assertEqual(conversions, [])

    def test_finds_nothing_once_the_originals_are_gone(self):
        # Arrange
        self.write("media/videos/curly_seq.ogv")
        self.write("media/music/silver_music/on_start.flac")

        # Act
        conversions = convert_assets.find_conversions(self.game)

        # Assert
        self.assertEqual(conversions, [])


class UpToDateTests(GameFolderTestCase):
    def test_newer_output_is_up_to_date(self):
        # Arrange
        conversion = self.effect()

        # Act / Assert
        self.assertTrue(convert_assets.is_up_to_date(conversion))

    def test_older_output_is_out_of_date(self):
        # Arrange
        conversion = self.effect(converted_age_seconds=7200)

        # Act / Assert
        self.assertFalse(convert_assets.is_up_to_date(conversion))

    def test_missing_output_is_out_of_date(self):
        # Arrange
        conversion = self.effect(converted=False)

        # Act / Assert
        self.assertFalse(convert_assets.is_up_to_date(conversion))


class CheckConversionTests(GameFolderTestCase):
    def test_verified_when_up_to_date_and_passing(self):
        # Arrange
        conversion = self.effect()

        # Act
        check = deleter.check_conversion(FakeVerifier(), conversion)

        # Assert
        self.assertEqual(check.status, deleter.VERIFIED)

    def test_not_converted_without_the_output(self):
        # Arrange
        conversion = self.effect(converted=False)

        # Act
        check = deleter.check_conversion(FakeVerifier(), conversion)

        # Assert
        self.assertEqual(check.status, deleter.NOT_CONVERTED)

    def test_out_of_date_when_the_output_is_older(self):
        # Arrange
        conversion = self.effect(converted_age_seconds=7200)

        # Act
        check = deleter.check_conversion(FakeVerifier(), conversion)

        # Assert
        self.assertEqual(check.status, deleter.OUT_OF_DATE)

    def test_failing_keeps_the_verification_error(self):
        # Arrange
        conversion = self.effect()

        # Act
        check = deleter.check_conversion(FakeVerifier(RuntimeError("frames differ")), conversion)

        # Assert
        self.assertEqual((check.status, check.reason), (deleter.FAILING, "frames differ"))

    def test_counts_the_original_bytes(self):
        # Arrange
        conversion = self.effect()

        # Act
        check = deleter.check_conversion(FakeVerifier(), conversion)

        # Assert
        self.assertEqual(check.original_bytes, len(ORIGINAL_BYTES))


class DeleteOriginalsTests(GameFolderTestCase):
    def test_deletes_a_verified_original(self):
        # Arrange
        conversion = self.effect()
        checks = [deleter.check_conversion(FakeVerifier(), conversion)]

        # Act
        deleter.delete_originals(checks)

        # Assert
        self.assertFalse(conversion.source.exists())

    def test_keeps_the_converted_file(self):
        # Arrange
        conversion = self.effect()
        checks = [deleter.check_conversion(FakeVerifier(), conversion)]

        # Act
        deleter.delete_originals(checks)

        # Assert
        self.assertTrue(conversion.target.exists())

    def test_keeps_a_failing_original(self):
        # Arrange
        conversion = self.effect()
        checks = [deleter.check_conversion(FakeVerifier(ValueError("bad Ogg")), conversion)]

        # Act
        deleter.delete_originals(checks)

        # Assert
        self.assertTrue(conversion.source.exists())

    def test_keeps_an_unconverted_original(self):
        # Arrange
        conversion = self.effect(converted=False)
        checks = [deleter.check_conversion(FakeVerifier(), conversion)]

        # Act
        deleter.delete_originals(checks)

        # Assert
        self.assertTrue(conversion.source.exists())


class SummaryTests(unittest.TestCase):
    @staticmethod
    def check(kind, status, original_bytes):
        conversion = convert_assets.Conversion(kind, Path("x.wav"), Path("x.ogg"))
        return deleter.ConversionCheck(conversion, status, "", original_bytes)

    def test_bytes_to_free_counts_only_verified_originals(self):
        # Arrange
        checks = [self.check(convert_assets.EFFECT_KIND, deleter.VERIFIED, 100), self.check(convert_assets.EFFECT_KIND, deleter.FAILING, 7),
                  self.check(convert_assets.MUSIC_KIND, deleter.VERIFIED, 20)]

        # Act / Assert
        self.assertEqual(deleter.bytes_to_free(checks), 120)

    def test_counts_each_kind_and_status(self):
        # Arrange
        checks = [self.check(convert_assets.VIDEO_KIND, deleter.VERIFIED, 1), self.check(convert_assets.VIDEO_KIND, deleter.VERIFIED, 1),
                  self.check(convert_assets.VIDEO_KIND, deleter.NOT_CONVERTED, 1)]

        # Act
        counts = deleter.count_by_kind_and_status(checks)

        # Assert
        self.assertEqual((counts[(convert_assets.VIDEO_KIND, deleter.VERIFIED)], counts[(convert_assets.VIDEO_KIND, deleter.NOT_CONVERTED)]), (2, 1))

    def test_dry_run_summary_says_what_it_would_delete(self):
        # Arrange
        checks = [self.check(convert_assets.MUSIC_KIND, deleter.VERIFIED, 1), self.check(convert_assets.MUSIC_KIND, deleter.OUT_OF_DATE, 1)]

        # Act
        last_line = deleter.summary_lines(checks, deleted=False)[-1]

        # Assert
        self.assertTrue(last_line.startswith("would delete 1 of 2 originals"))


class ProgressFramesTests(unittest.TestCase):
    def test_reads_the_last_frame_count(self):
        # Arrange
        progress = "frame=120\nfps=0.0\nprogress=continue\nframe=300\nprogress=end\n"

        # Act / Assert
        self.assertEqual(convert_assets.parse_progress_frames(progress), 300)

    def test_rejects_progress_without_frames(self):
        # Arrange / Act / Assert
        with self.assertRaises(ValueError):
            convert_assets.parse_progress_frames("progress=end\n")


class BinkHeaderTests(GameFolderTestCase):
    def test_reads_the_frame_count(self):
        # Arrange
        header = b"BIKi" + (5303524).to_bytes(4, "little") + (300).to_bytes(4, "little") + bytes(32)
        bik = self.write("media/videos/curly_seq.bik", header)

        # Act / Assert
        self.assertEqual(convert_assets.bink_frame_count(bik), 300)


if __name__ == "__main__":
    unittest.main()
