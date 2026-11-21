# Audio: mix levels

## Mix levels — DirectSound's headroom per voice path
The sound manager's voices (0xc4600..0xc4a05) and DirectSound's defaults (linked into default.xbe) put each kind of sound on a path with its own fixed attenuation; xemu's VP mixes them the same
way (vp.c: a 3D voice's HRTF bins use the HRTF headroom 0, every other mixbin its submix headroom 1 = /2).
- Mono sounds (positional or not): one of 26 3D voices (0xc47de, DSBCAPS_CTRL3D, mixbins 6 8 7 9 10): headroom 0,
  the HRTF bins unattenuated: **0 dB**. In Stereo mode a 3D voice's rear volume is DSBVOLUME_MIN, so the downmix adds nothing to it.
- Each 3D voice record carries six 2D speaker voices (0xc484e..0xc49f0, flags 0: FC, LFE, BR, BL, FR + FC -500,
  FL + FC -500). The parser's CENTRE / LFE / LS / RS / RIGHT / LEFT set +0x91..+0x96 (LS sub-voice 3 on BACK_RIGHT,
  RS 4 on BACK_LEFT), NOPOSITION sets +0x91 / +0x95 / +0x96, and sound_play_ui (0xc4108) plays the sound's data on
  every flagged one at the record's volume (0xcaf70, through 0xc7990: (vol + 10000) / 10000 x (option + 10000) -
  10000 mB). So a NOPOSITION sound (menus, INCOMING, LOWENERGY, CHING, the turbo) is the 3D voice straight ahead
  plus centre and both fronts at -12 dB: downmixed, 0.63 per side unfiltered over the HRTF's 0.42. Argon reads
  the speaker keywords (and ROUTETO's targets: GN_Mimpact_02 on all six) into SoundOptions::speakers; a single-file
  sound's VoiceRouter (audio\voice_router.hpp) plays its HRTF pair and, unfiltered, mix::speakerShare's six mixbins
  (AudioOutput::routingWithSpeakers; a positional one's share also takes its distance rolloff, the game's 2D voices
  would not; it also passes the crosstalk with the 3D voice). The track one-shots (countdown three / two / one / go,
  tunnel: NOMAIN LEFT RIGHT) play their L / R files on their speaker mixbins too. Not
  ported: D51_CENTRE / D51_LFE (Rlaunch), and what a NOMAIN + NOPOSITION sound's centre voice plays.
- Argon's master volume (the editor slider) defaults to 1 (it was 0.8, -1.9 dB on everything).
- Per-speaker files (LEFT / RIGHT / LS / RS, the 5.1 ambience): 2D voices, one per file (0xc484e..0xc49f0: FC, LFE,
  BR, BL, FR + FC -500, FL + FC -500): the 2D voice headroom 600 mB and the speaker mixbins' /2: **-12 dB**.
- Engine layers: 2D voices (600 mB) into their craft's 3D submix buffer (0x2010, SetHeadroom(100) at 0xc473c; the submix mixbin has headroom 0): **-7 dB**, and the 3D bins' -6 dB after the HRTF.
- Music: a 2D stream (0xdf7fe) with SetHeadroom(100) (0xdf854) on the speaker mixbins: **-7 dB**.
- The options' millibels (0x1a8564) come on top, as they are (argon used to keep its own levels at step 4).
- HRTF: the game links DirectSound's full HRTF only (the lite table at 0x14dd18 is inside the full one): 2222 filters
  of 31 tap codes + an interaural delay byte (32 bytes each, 0x1459d8) and a 61 x 31 index by azimuth / 3 and
  (elevation + 90) / 6 (0x156f98); tools\assets\extract_hrtf.py copies both into <game>\audio\hrtf.bin, which argon
  reads before falling back to the XBE (audio\hrtf.cpp). DirectSound turns the source's position into an azimuth and an
  elevation (piecewise 45 degree mapping of the dot products with the listener's front / right / top), rounds them
  (6 degree elevation; 3, 6 or 12 degree azimuth by elevation), mirrors the pair for the left side and gives the
  delay to the far ear. DirectSound writes the bytes to the VP as they are, and the
  VP keeps 12-bit taps: a byte is sign, 3-bit exponent, 4-bit mantissa, the tap
  (16 + mantissa) << (exponent - 1) (exponent 0: the mantissa), of 2048. Decoded so, the front pair #1170 (which
  DirectSound loads for a voice whose 3D processing is off) is two taps of 336 and close to flat
  (flat to 12 kHz, -7.5 dB); 2048 is the 12-bit taps' full scale. The 3D (crosstalk) mixbins the HRTF feeds keep the
  1-bit headroom of every mixbin but the submix one (the HRTF headroom 0 is a separate stage): **-6 dB** on every 3D voice (mix::kHrtfBinsMb, on its ears in
  AudioOutput::routingFor; the speaker voices' share keeps its level). Until 2026-11-20 the
  argon left it out and the engines and the emitters played too loud over the ambience on a real Xbox's grid. The DSP (audio\dsstdfx.bin) DMAs the speaker mixbufs to the output unscaled; the far ear rolls off, the near ear stays bright and about 4 dB louder, the rear is dull.
  xemu's VP (vp\hrtf.h) reads them as two's complement and normalises each ear to a sum of |taps| of 1, which gives
  every filter a ragged, dull response (the front about -10 dB with dips; argon did the same until 2026-11-20,
  and every 3D sound, menu clicks included, came out muffled). Taps and delay glide per sample (0.01, xemu's).
  Argon Options' HRTF can put MIT KEMAR in their place (audio\kemar.cpp, a viewer extra: argon/differences.md):
  tools\assets\convert_kemar.py turns the full set's normal-pinna impulses into 48 kHz, diffuse-field equalized,
  512-tap minimum-phase ears plus the far ear's onset delay, scaled so the front pair cut to 64 taps passes
  200 Hz..12 kHz at the -7.5 dB of filter #1170; the rest of the path (the -6 dB bins, 5.1's front mirror) is the same.
  Argon Options' KEMAR Taps cuts them to 64 (default), 128, 256 or 512, the last 8 faded out (cos^2). On the full set
  (2026-11-21, the converter prints it): the front pair -7.50 / -7.49 / -7.47 / -7.46 dB, the far ear's delay up to
  40.6 samples; the energy past the cut, worst ear / median, -8.6 / -18.3, -12.0 / -22.1, -13.6 / -25.0, -29.1 /
  -41.1 dB (the worst: the far ear below the horizon, quiet and long). The ear filter's cost follows the taps
  (hrtf::EarFilter: a delay line, then the taps over 64-frame blocks, each tap loaded once per 32 outputs with AVX2 /
  FMA, 16 with SSE where the CPU lacks them, audio\fir_block.cpp; the glide is a crossfade that ends at -100 dB):
  on a 2026 desktop (AVX2), per voice and second of sound, 0.35 / 0.43 ms at 31 / 64 taps, 0.6 / 0.9 / 1.5 ms at
  128 / 256 / 512, up to 2.4 times that while gliding; about 30 voices at 512 taps take 4.5 % of one core, 11 % all
  gliding (the older per-tap loop: 4.7 ms at 31 taps, 9.5 at 64, 75 at 512). Partitioned FFT convolution would not
  pay at 512 taps: without added latency (the 3D ears would trail the same sound's 2D speaker voices) it keeps a
  time-domain head of the first block's taps, and the FFTs cost about what the rest of the taps do.
  xemu mixes a mono 3D voice's left ear into every one of its bins (vp.c `samples[i][b % channels]`, channels 1), so
  both speakers get the left ear there; argon gives each speaker its own ear (left ear left).
- Crosstalk canceller (a program in audio\dsstdfx.bin; on with speakers, off with headphones) on the 3D bins on their way
  to the speakers: each side's output, 11 samples late at 48 kHz, through y = 0.924 y - 0.0585 x into the other. One
  program serves both pairs (4 in, 4 out): in 5.1 the rear 3D pair gets its own.
- Argon: audio\mix_paths.hpp; a Pcm's speakerFiles (mixSpeakerFiles: more than one file, kept as the six speaker
  mixbins); Voice plays a game sound (Bus::Sfx, Bus::World) of one file through its own VoiceRouter node (the HRTF of
  audio\hrtf.cpp, the tables read from default.xbe at the front end and at a race's load) into the 3D groups, one of
  speaker files at -12 dB into the 2D groups, anything on Bus::Main (and the music stream) into the front pair
  (audio_output.hpp VoicePath, world_effects.cpp: the listener filter on World, the crosstalk on the 3D ones, the speaker
  mixbins downmixed for stereo); RaceSfx, the emitters and the other crafts' engines hand each voice its source's routing
  every frame (no more equal-power pan); every other voice keeps the listener's. The engines and the music add their
  headroom where their volume is set; the emitters play at the SFX volume (not checked in the game which option they
  follow), the story scenes' cut_ambient at the SFX option less 500 mB (0xca540 / 0xc7990), the menus' ui_amb3 at the
  SFX option (sound_play_ui) and the loading screen's ui_levelload at the Music Volume (0xca610 / 0xdee90). The mix runs at 48 kHz.
- Not ported: a sound with a main file and speaker files (water_enter, rain_windshield, Rlaunch) plays them all mixed
  on the 2D path (the game: the main file on a 3D voice, the others on 2D voices); the folders' NOMAIN / LEFT /
  RIGHT flags are not read.
- Stereo downmix: with the dashboard on Stereo, DirectSound has the Dolby DSP downmix the six mixbins to two
  (the digital output an XboxHD+ takes, compared on 2026-11-20). The downmix
  program's exact coefficients are unknown, so argon takes the standard Lo / Ro: centre and rears at -3 dB, LFE dropped
  (audio\mix_paths.hpp stereoDownmix, foldSpeakers). A per-speaker sound's _L / _R files thus reach both sides through
  their -5 dB centre copies (L: 1.40 left, 0.40 right), _LS / _RS their side at 0.71: the pickups about 4..5 dB louder
  than the old fold (LS / RS in full, no centre). The int16 scaling no longer costs level: Pcm::storedScale, played back
  by Voice (the folded pickups had lost 2.6 dB to it). Since the 5.1 work the story scenes' 5.1 ambience (cut_ambient's
  Amb_N.L / R / C / Ls / Rs / LFE) takes the same speaker mixbins (mix::speakerFileGains): L / R with their centre
  copies, Ls / Rs on their side only (the old fold gave them 0.8 / 0.2) and C at 0.71 as before.
- 5.1 (Argon Options' Speakers, settings.json surroundSound; taken when the audio opens): the dashboard's Dolby Digital,
  DirectSound's surround mode (the AC3 encoder on all six mixbins). The output runs at six
  channels, FL FR FC LFE BL BR (mix::Speaker), and the mixbins reach them as they are:
  - a 3D voice's HRTF pair goes to the front 3D pair at its front volume and to the rear pair at its rear one
    (the front volume on the fronts, the rear volume on the rears). DirectSound's front / rear split (in stereo
    the rear is DSBVOLUME_MIN): factor = 0.5 (1 + (|azimuth| / 90 - 1)(1 - |elevation| / 90)), then x 2 - 0.5 clamped
    to 0..1, so full front until 45 degrees off the front axis; inside 0.5 m it crossfades to 0.5 at the head; front
    1000 log10(1 - factor) mB, rear 1000 log10(factor) mB (power-preserving). Behind the listener
    the filter pick (0x13dd6c) takes the front mirror's filters in surround:
    hrtf::surroundGains, mirroredToFront.
  - a NOPOSITION voice sits at the listener: sound_play_ui (0xc4017) gives it 0x520e10, the position 0xc7b70 hands the
    listener, so its distance is 0 and its front and rear pairs each play at -3 dB; its speaker voices add the centre
    and both fronts. Argon's own engine stays ahead (hrtf::kAheadDistance: the craft ahead of the camera).
  - the 2D speaker voices and per-speaker files play on their own speakers (the LFE ones too, at the level the files
    give: whether the AC3 stream's LFE is played back 10 dB up is the receiver's business); the music on the fronts.
  Not checked against a 5.1 capture. An output device with fewer channels gets miniaudio's own downmix of the six.
