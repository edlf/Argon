// The story scenes' speech lines (the XBE's table 0x1a8768; the rows: movie\speech_lines.cpp). A speech object
// (NMS type 9) names its line by key (+0x160); 0x96a60 loads the line's WAV and face animation, and the scene shows
// cutscenes.str's string of the line's id while it plays (docs\sequences\README.md "Speech").
#pragma once
#include <array>
#include <optional>
#include <string>

namespace argon::story
{

struct SpeechLine
{
    int stringId;            // cutscenes.str
    int speaker;             // the pilot whose portrait shows and whose face animates (0..15)
    const char *key;         // "IDS_SILVER_TARA"
    const char *wav;         // under data\movies\speech\: "tara\tara_1.wav"
    const char *faceAnimation;   // under data\characters\: "tara\tara face 01.cms" ("" none)
};

constexpr size_t kSpeechLineCount = 248;
extern const std::array<SpeechLine, kSpeechLineCount> kSpeechLines;

// 0x9aff0 / 0x9b070 / 0x9af70: the first line whose key matches, comparing up to the first character below '!' of
// either key (so an empty key takes the first line, as silver_tara.nms's speech does); none: nullopt
std::optional<SpeechLine> speechLineFor(const std::string &key);
bool speechKeysMatch(const std::string &objectKey, const char *lineKey);

} // namespace argon::story
