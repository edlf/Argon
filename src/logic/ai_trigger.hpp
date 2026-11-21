// Argon: the AI's trigger (drone.cpp in the game: 0x6e9e0 each frame, docs\weapons\ai.md "AI firing").
//   wants    lock-on weapons with a target in the lock list (0x6e660), other weapons with a target within 5 m of the
//            line ahead (0x6e380), shields when locked on (0x6e1e0, speed 2..4 or a nemesis) or under 40 % (0x6e2b0);
//            the first two only while the drone is less than 5 % of a lap behind the player (+0x1808) or is the
//            nemesis (+0x9f9), which looks at the player only; the player listed while finished stops red and blue
//            (+0x1800, 0x597314)
//   trigger  red / blue fire once their clock (+0x17f0 / +0x17f4, counting while wanted) passes its delay; yellow at
//            once; any fire draws new delays (aiFireDelay: 2.0 .. 4.48 s by speed class)
// What a colour holds (the armed record, a shot still out) and firing are the caller's.
#pragma once
#include "logic/race_crafts.hpp"
#include "logic/viewer_rand.hpp"

#include <optional>
#include <vector>

namespace argon::weapons
{

// per colour: red, blue, yellow
struct AiWants
{
    bool red = false, blue = false, yellow = false;
};
using AiFire = AiWants;

// the trigger (drone 0x6e9e0): the red / blue clocks (+0x17f0 / +0x17f4) and their delays (+0x17f8 / +0x17fc)
struct AiTrigger
{
    float redClock = 0, blueClock = 0, redDelay = 0, blueDelay = 0;
};

// what one colour's armed weapon is (the record a level up in the game, the same weapon in the data)
struct ArmedColour
{
    bool armed = false;      // a record for the colour's level
    bool locksOn = false;    // +0x150
    bool shield = false;     // a TSHIELD (class 2) with shield resistance
    bool inFlight = false;   // its last volley, shot or shield still active
};
struct AiArmament
{
    ArmedColour red, blue, yellow;
};

// the drone's situation this frame
struct AiSituation
{
    bool engages = false;              // aims at targets at all (aiEngages)
    bool hasTargets = false;           // its targets (aiTargets) are not empty
    bool targetInLine = false;         // targetInLine over its targets
    bool threatened = false;           // aiThreatened
    bool holdsForFinishedPlayer = false;   // the player has finished and is in its lock list
};

// what puts a drone's shields up
struct AiThreat
{
    int speedClass = 0;
    bool nemesis = false, lockedOn = false;   // locked on: in a player's lock lists or a shot's target
    float shieldPercent = 100.f;
};

// a nemesis always; else while less than 5 % of a lap behind the player (percentAhead: its race distance minus the
// player's, in percent of a lap; none without a craft)
bool aiEngages(bool nemesis, std::optional<float> percentAhead);
// 0x6e8d0 / 0x6e950: the lock list, or for a nemesis the player alone when listed (mask 1)
std::vector<int> aiTargets(const std::vector<int> &listed, bool nemesis);
// 0x6e380: a target within 5 m of the line ahead of the craft in `slot` and 500 m of it
bool targetInLine(int slot, const std::vector<int> &targets, const RaceCrafts &crafts);
bool aiThreatened(const AiThreat &threat);
AiWants aiWants(const AiSituation &situation, const AiArmament &armament);
// 0x6e9e0: the clocks count while wanted (else go back to 0); returns the colours that fire now
AiFire advanceTrigger(AiTrigger &trigger, const AiWants &wants);
// 0x6ebfc, after any fire: speed 1 / 2: 2.5 + (rand % 100) x 0.02; speed 3 / 4 / 5: 2.4 / 2.2 / 2.0 + (rand % 100) x 0.01
float aiFireDelay(int speedClass, ViewerRand &rand);

}  // namespace argon::weapons
