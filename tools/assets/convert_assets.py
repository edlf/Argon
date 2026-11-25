"""Converts the game's videos, music and sound effects with ffmpeg, next to the originals, for the viewer (the game's own
files are never changed):

  videos         media/videos/*.bik, Locale/<language>/intro.bik -> <name>.ogv: the picture as Theora and the soundtrack
                 (when the video has one) as Vorbis, muxed in Ogg. The viewer plays it in place of <name>.bik.
  music          media/music/<track>_music/*.wav -> <name>.flac, lossless: only the samples of the data chunk (the chunks
                 after it, LIST tags and Sound Forge's bext / regn / ... blocks, are not sound). The Xbox ADPCM ones
                 (frontend_music, levelload_music) are decoded by argon's decoder (argon_decode) and cut to their fact
                 length. Each FLAC is decoded back and must equal the source's samples bit for bit.
  sound effects  audio/**/*.wav (Xbox ADPCM) -> <name>.ogg, Ogg Vorbis at 256 kbit/s, or libvorbis's highest bit rate
                 for the format when it rejects 256 kbit/s (mono 22050 Hz, all of the game's effects: its limit is
                 90 kbit/s). Decoded by argon's decoder (argon_decode), every ADPCM block whole as the viewer keeps it
                 (loops wrap on whole blocks); the fact chunk's sample count is the comment PLAYLENGTH (where a
                 one-shot stops), the smpl chunk's loop the comments LOOPSTART (first frame) and LOOPLENGTH (frames).
                 Each Ogg is decoded back by the viewer and must give the .wav's length, play length and loop.

The viewer loads <name>.flac / <name>.ogg in place of a game .wav when it is there (audio\\audio.cpp loadGameSound), and
lists a folder's sounds by those names when the .wav is gone (audio\\game_sound_files.hpp).

usage: python tools/assets/convert_assets.py [--force] [--ffmpeg <path to ffmpeg>] [--decoder <path to argon_decode>] [--game <folder>]

Only the originals still there are converted (tools/assets/delete_converted_originals.py deletes the verified ones); with none
left there is nothing to do. An output newer than its source is skipped unless --force (a video's .ogv also needs the
soundtrack when the .bik has one; the separate <name>.ogg soundtrack of earlier video conversions is deleted). ffmpeg
decodes Bink itself and encodes FLAC, Theora and Vorbis; install it from ffmpeg.org (Windows: winget install
Gyan.FFmpeg; Linux: the distribution's ffmpeg package, built with libtheora and libvorbis). argon_decode is argon's sound
decoder, built by build.bat (default build\\argon_decode.exe).
"""
import argparse
import functools
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
VIDEO = ["-map", "0:v:0", "-c:v", "libtheora", "-q:v", "9"]   # quality 0..10
VIDEO_AUDIO = ["-map", "0:a:0", "-c:a", "libvorbis", "-q:a", "6", "-ac", "2"]
VORBIS_ID_HEADER = b"\x01vorbis"
HEADER_PAGES_BYTES = 64 * 1024   # the streams' identification pages open the file
BINK_HEADER_BYTES = 44
FLAC = ["-c:a", "flac", "-compression_level", "8"]
WANTED_VORBIS_KBITS = 256
LOWEST_VORBIS_KBITS = 8
WAV_PCM = 1
SAMPLE_BYTES = 2   # 16-bit samples
ENCODE_TIMEOUT_SECONDS = 1800
DECODE_REPORT = re.compile(r"play (\d+), loop \[(\d+), (\d+)\)")
PROGRESS_FRAME = re.compile(r"^frame=(\d+)\s*$", re.MULTILINE)
VIDEO_KIND, MUSIC_KIND, EFFECT_KIND = "video", "music", "sound effect"
TARGET_EXTENSIONS = {VIDEO_KIND: ".ogv", MUSIC_KIND: ".flac", EFFECT_KIND: ".ogg"}


@dataclass(frozen=True)
class Conversion:
    """one original of the game (kind: VIDEO_KIND, MUSIC_KIND or EFFECT_KIND) and the file written next to it"""
    kind: str
    source: Path
    target: Path


@dataclass
class WavFile:
    """the chunks of a game .wav the conversion needs"""
    format_tag: int = 0
    channels: int = 0
    rate: int = 0
    bits: int = 0
    data: bytes = b""
    fact_frames: int = 0   # 0: no fact chunk
    loop_start: int = 0    # the smpl chunk's first loop, [start, end] in frames; both 0: none
    loop_end: int = 0


@dataclass
class DecodedSound:
    """the viewer's decode of a sound: all its samples, its one-shot length (0: all) and its loop [begin, end)"""
    pcm: "Pcm"
    play_frames: int
    loop_begin: int
    loop_end: int


@dataclass
class Pcm:
    """16-bit little-endian interleaved samples"""
    rate: int
    channels: int
    samples: bytes

    def frames(self):
        return len(self.samples) // (SAMPLE_BYTES * self.channels)


def read_chunks(contents):
    """(id, body) for each chunk of a RIFF WAVE file"""
    if contents[:4] != b"RIFF" or contents[8:12] != b"WAVE":
        raise ValueError("not a RIFF WAVE file")
    chunks = []
    offset = 12
    while offset + 8 <= len(contents):
        chunk_id, size = struct.unpack_from("<4sI", contents, offset)
        chunks.append((chunk_id, contents[offset + 8:offset + 8 + size]))
        offset += 8 + size + (size & 1)
    return chunks


def read_sampler_loop(body, wav):
    """smpl: 36 bytes of header (+28 the number of loops), then 24 per loop: id, type, start, end, fraction, count"""
    loop_count, = struct.unpack_from("<I", body, 28)
    if loop_count and len(body) >= 36 + 24:
        _, _, wav.loop_start, wav.loop_end = struct.unpack_from("<IIII", body, 36)


def parse_wav(contents):
    wav = WavFile()
    for chunk_id, body in read_chunks(contents):
        if chunk_id == b"fmt ":
            wav.format_tag, wav.channels, wav.rate, _, _, wav.bits = struct.unpack_from("<HHIIHH", body)
        elif chunk_id == b"fact":
            wav.fact_frames, = struct.unpack_from("<I", body)
        elif chunk_id == b"smpl":
            read_sampler_loop(body, wav)
        elif chunk_id == b"data":
            wav.data = body
    if not wav.channels or not wav.data:
        raise ValueError("no fmt or data chunk")
    return wav


def effect_comments(wav):
    """the Vorbis comments: PLAYLENGTH the fact count (a one-shot's length), the smpl loop [start, end] as LOOPSTART and
    LOOPLENGTH (the viewer's loop is [start, end + 1), wav.cpp)"""
    comments = {"PLAYLENGTH": wav.fact_frames} if wav.fact_frames else {}
    if wav.loop_end > wav.loop_start:
        comments.update({"LOOPSTART": wav.loop_start, "LOOPLENGTH": wav.loop_end + 1 - wav.loop_start})
    return comments


def parse_decode_report(report):
    """argon_decode's line: "... play <frames>, loop [<begin>, <end>)" -> (play frames, loop begin, loop end)"""
    found = DECODE_REPORT.search(report)
    if not found:
        raise ValueError(f"unexpected argon_decode output: {report!r}")
    return tuple(int(number) for number in found.groups())


def parse_progress_frames(progress):
    """ffmpeg -progress's last "frame=<n>" line: the frames it has written"""
    frames = PROGRESS_FRAME.findall(progress)
    if not frames:
        raise ValueError(f"no frame count in ffmpeg's progress: {progress!r}")
    return int(frames[-1])


def first_frames(pcm, frames):
    return Pcm(pcm.rate, pcm.channels, pcm.samples[:frames * pcm.channels * SAMPLE_BYTES])


def find_tool(explicit, name, hint):
    found = explicit or shutil.which(name)
    if not found or not Path(found).exists():
        sys.exit(f"{name} not found: {hint}")
    return Path(found)


def default_decoder():
    return PROJECT / "build" / ("argon_decode.exe" if os.name == "nt" else "argon_decode")


class Converter:
    def __init__(self, ffmpeg, decoder, force):
        self.ffmpeg = ffmpeg
        self.decoder = decoder
        self.force = force

    def needs_update(self, conversion):
        return self.force or not is_up_to_date(conversion)

    # ------------------------------------------------------------------ decoding
    def decode_with_viewer(self, path):
        """the viewer's own decode (Xbox ADPCM, FLAC, Ogg Vorbis): all its samples as PCM16, its play length and loop"""
        with tempfile.TemporaryDirectory() as folder:
            decoded = Path(folder) / "decoded.wav"
            command = [str(self.decoder), str(path), str(decoded)]
            report = subprocess.run(command, check=True, capture_output=True, text=True, timeout=ENCODE_TIMEOUT_SECONDS).stdout
            wav = parse_wav(decoded.read_bytes())
        return DecodedSound(Pcm(wav.rate, wav.channels, wav.data), *parse_decode_report(report))

    def music_pcm(self, path, wav):
        """PCM16 as stored; anything else (Xbox ADPCM) by the viewer, cut to the fact length the music plays"""
        if wav.format_tag == WAV_PCM and wav.bits == 16:
            return Pcm(wav.rate, wav.channels, wav.data)
        decoded = self.decode_with_viewer(path).pcm
        return first_frames(decoded, wav.fact_frames) if wav.fact_frames else decoded

    def decode_with_ffmpeg(self, path):
        command = [str(self.ffmpeg), "-v", "error", "-i", str(path), "-f", "s16le", "-acodec", "pcm_s16le", "-"]
        return subprocess.run(command, check=True, capture_output=True, timeout=ENCODE_TIMEOUT_SECONDS).stdout

    def ogv_frame_count(self, ogv):
        """the picture's frames, counted by ffmpeg copying the stream (no decode)"""
        command = [str(self.ffmpeg), "-v", "error", "-i", str(ogv), "-map", "0:v:0", "-c", "copy", "-f", "null", "-", "-progress", "pipe:1"]
        progress = subprocess.run(command, check=True, capture_output=True, text=True, timeout=ENCODE_TIMEOUT_SECONDS).stdout
        return parse_progress_frames(progress)

    # ------------------------------------------------------------------ encoding
    def encode_pcm(self, pcm, options, target, container):
        partial = target.with_name(target.stem + ".part" + target.suffix)
        command = [str(self.ffmpeg), "-y", "-v", "error", "-f", "s16le", "-ar", str(pcm.rate), "-ac", str(pcm.channels), "-i", "pipe:0",
                   *options, "-f", container, str(partial)]
        try:
            subprocess.run(command, input=pcm.samples, check=True, timeout=ENCODE_TIMEOUT_SECONDS)
            partial.replace(target)
        finally:
            partial.unlink(missing_ok=True)

    def vorbis_accepts(self, rate, channels, kbits):
        command = [str(self.ffmpeg), "-v", "quiet", "-f", "lavfi", "-i", f"anullsrc=r={rate}", "-t", "0.1", "-ac", str(channels),
                   "-c:a", "libvorbis", "-b:a", f"{kbits}k", "-f", "null", "-"]
        return subprocess.run(command, timeout=60).returncode == 0

    def highest_rejected_half(self, rate, channels):
        """halving 256 kbit/s until libvorbis accepts it (it takes a range of bit rates for each format)"""
        rejected = WANTED_VORBIS_KBITS
        while rejected // 2 >= LOWEST_VORBIS_KBITS:
            if self.vorbis_accepts(rate, channels, rejected // 2):
                return rejected
            rejected //= 2
        raise RuntimeError(f"libvorbis rejects {channels} channel(s) at {rate} Hz")

    @functools.lru_cache(maxsize=None)
    def vorbis_kbits(self, rate, channels):
        """256 kbit/s, else the highest bit rate libvorbis accepts for the format"""
        if self.vorbis_accepts(rate, channels, WANTED_VORBIS_KBITS):
            return WANTED_VORBIS_KBITS
        rejected = self.highest_rejected_half(rate, channels)
        low, high = rejected // 2, rejected - 1
        while low < high:
            middle = (low + high + 1) // 2
            if self.vorbis_accepts(rate, channels, middle):
                low = middle
            else:
                high = middle - 1
        print(f"libvorbis rejects {WANTED_VORBIS_KBITS} kbit/s for {channels} channel(s) at {rate} Hz: using {low} kbit/s, its highest")
        return low

    # ------------------------------------------------------------------ music and sound effects
    def convert_music(self, source, target):
        pcm = self.music_pcm(source, parse_wav(source.read_bytes()))
        self.encode_pcm(pcm, FLAC, target, "flac")
        self.check_same_samples(target, pcm)

    def check_same_samples(self, target, pcm):
        if self.decode_with_ffmpeg(target) != pcm.samples:
            raise RuntimeError(f"{target}: the FLAC does not decode to the source's samples")

    def convert_effect(self, source, target):
        """the whole decode, the padded last block included (loops wrap on whole blocks); one-shots stop at PLAYLENGTH"""
        wav = parse_wav(source.read_bytes())
        original = self.decode_with_viewer(source)
        pcm = original.pcm
        options = ["-c:a", "libvorbis", "-b:a", f"{self.vorbis_kbits(pcm.rate, pcm.channels)}k"]
        for key, value in effect_comments(wav).items():
            options += ["-metadata", f"{key}={value}"]
        self.encode_pcm(pcm, options, target, "ogg")
        check_same_sound(target, self.decode_with_viewer(target), original)

    def convert_sounds(self, conversions, convert):
        for conversion in conversions:
            if self.needs_update(conversion):
                print(f"converting  {conversion.target}", flush=True)
                convert(conversion.source, conversion.target)
            else:
                print(f"up to date  {conversion.target}")
        report_sizes(conversions)

    # ------------------------------------------------------------------ videos
    def convert_videos(self, conversions):
        for conversion in conversions:
            if self.force or not video_up_to_date(conversion.source, conversion.target):
                print(f"converting  {conversion.target}", flush=True)
                self.encode_video(conversion.source, conversion.target)
            else:
                print(f"up to date  {conversion.target}")
            remove_separate_soundtrack(conversion.source)

    def encode_video(self, bik, ogv):
        options = VIDEO + (VIDEO_AUDIO if bink_has_audio(bik) else [])
        partial = ogv.with_name(ogv.stem + ".part" + ogv.suffix)
        command = [str(self.ffmpeg), "-y", "-v", "error", "-i", str(bik), *options, "-f", "ogg", str(partial)]
        try:
            subprocess.run(command, check=True, timeout=ENCODE_TIMEOUT_SECONDS)
            partial.replace(ogv)
        finally:
            partial.unlink(missing_ok=True)

    # ------------------------------------------------------------------ verifying an existing output
    def verify(self, conversion):
        """raises when the output does not stand for its original: the check each conversion ends with"""
        verifications = {VIDEO_KIND: self.verify_video, MUSIC_KIND: self.verify_music, EFFECT_KIND: self.verify_effect}
        verifications[conversion.kind](conversion.source, conversion.target)

    def verify_video(self, bik, ogv):
        """the soundtrack when the .bik has one, and as many frames of picture"""
        if bink_has_audio(bik) and not has_soundtrack(ogv):
            raise RuntimeError(f"{ogv}: no soundtrack; the .bik has one")
        frames, expected = self.ogv_frame_count(ogv), bink_frame_count(bik)
        if frames != expected:
            raise RuntimeError(f"{ogv}: {frames} frames; the .bik has {expected}")

    def verify_music(self, source, target):
        self.check_same_samples(target, self.music_pcm(source, parse_wav(source.read_bytes())))

    def verify_effect(self, source, target):
        check_same_sound(target, self.decode_with_viewer(target), self.decode_with_viewer(source))


def check_same_sound(target, converted, original):
    """the viewer plays the Ogg as the .wav: the same length, one-shot length and loop"""
    lengths = (converted.pcm.frames(), converted.play_frames, converted.loop_begin, converted.loop_end)
    expected = (original.pcm.frames(), original.play_frames, original.loop_begin, original.loop_end)
    if lengths != expected:
        raise RuntimeError(f"{target}: frames, play frames, loop {lengths}; the .wav has {expected}")


def read_bink_header(bik):
    with bik.open("rb") as f:
        return f.read(BINK_HEADER_BYTES)


def bink_has_audio(bik):
    """the Bink header: +40 the number of audio tracks"""
    tracks, = struct.unpack_from("<I", read_bink_header(bik), 40)
    return tracks > 0


def bink_frame_count(bik):
    """the Bink header: +8 the number of frames"""
    frames, = struct.unpack_from("<I", read_bink_header(bik), 8)
    return frames


def has_soundtrack(ogv):
    with ogv.open("rb") as f:
        return VORBIS_ID_HEADER in f.read(HEADER_PAGES_BYTES)


def is_up_to_date(conversion):
    """the output is there and not older than its original"""
    target = conversion.target
    return target.exists() and target.stat().st_mtime >= conversion.source.stat().st_mtime


def video_up_to_date(bik, ogv):
    if not is_up_to_date(Conversion(VIDEO_KIND, bik, ogv)):
        return False
    return has_soundtrack(ogv) or not bink_has_audio(bik)


def remove_separate_soundtrack(bik):
    old = bik.with_suffix(".ogg")
    if old.exists():
        print(f"removing    {old}")
        old.unlink()


def report_sizes(conversions):
    if not conversions:
        return
    megabytes = 1024 * 1024
    before = sum(conversion.source.stat().st_size for conversion in conversions) / megabytes
    after = sum(conversion.target.stat().st_size for conversion in conversions) / megabytes
    print(f"{len(conversions)} files: {before:.1f} MB of .wav, {after:.1f} MB of {conversions[0].target.suffix}")


def files_with_extension(folder, extension):
    if not folder.is_dir():
        return []
    return sorted(path for path in folder.rglob("*") if path.is_file() and path.suffix.lower() == extension)


def find_conversions(game):
    """every original still in the game's folder that the converter handles: the videos (.bik anywhere), the music
    (media/music/**/*.wav) and the sound effects (audio/**/*.wav); nothing else (data/movies/speech stays .wav)"""
    originals = [(VIDEO_KIND, files_with_extension(game, ".bik")),
                 (MUSIC_KIND, files_with_extension(game / "media" / "music", ".wav")),
                 (EFFECT_KIND, files_with_extension(game / "audio", ".wav"))]
    return [Conversion(kind, source, source.with_suffix(TARGET_EXTENSIONS[kind])) for kind, sources in originals for source in sources]


def conversions_of_kind(conversions, kind):
    return [conversion for conversion in conversions if conversion.kind == kind]


def add_tool_arguments(parser):
    """--ffmpeg, --decoder and --game (shared with tools/assets/delete_converted_originals.py)"""
    parser.add_argument("--ffmpeg", help="path to ffmpeg")
    parser.add_argument("--decoder", default=str(default_decoder()), help="path to argon_decode, argon's sound decoder (default: build\\argon_decode.exe)")
    parser.add_argument("--game", default=str(PROJECT / "game_data"), help="the game's folder (default: game_data)")


def find_tools(args):
    """(ffmpeg, argon_decode) from the arguments; exits when one is missing"""
    ffmpeg = find_tool(args.ffmpeg, "ffmpeg", "install it (ffmpeg.org) or pass --ffmpeg <path to ffmpeg>")
    decoder = find_tool(args.decoder, "argon_decode", "build it (build.bat) or pass --decoder <path to argon_decode>")
    return ffmpeg, decoder


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--force", action="store_true", help="convert even when the outputs are up to date")
    add_tool_arguments(parser)
    args = parser.parse_args()
    conversions = find_conversions(Path(args.game))
    if not conversions:
        print(f"nothing to convert: no .bik or .wav originals under {args.game}")
        return
    converter = Converter(*find_tools(args), args.force)
    videos = conversions_of_kind(conversions, VIDEO_KIND)
    converter.convert_videos(videos)
    print(f"videos: {len(videos)}")
    print("music:", flush=True)
    converter.convert_sounds(conversions_of_kind(conversions, MUSIC_KIND), converter.convert_music)
    print("sound effects:", flush=True)
    converter.convert_sounds(conversions_of_kind(conversions, EFFECT_KIND), converter.convert_effect)


if __name__ == "__main__":
    main()
