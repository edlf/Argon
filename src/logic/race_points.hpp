// Argon: the points a race earns as the HUD keeps them (HUD elements 0x66a4 "Score" / 0x68b0 value). A HUD event
// (0x83f00: kind, points, pilot) adds the points to the target, which the shown score climbs to (gamehud_update
// 0x496f0), and queues a popup (element +0x9380: 0x4cb00 add, 0x4cc30 update); the points per kind are the POINTSMENU
// breakdown (ui\score.cpp). Kinds (popup labels, table 0x1b9748): 0 attack, 2 kill, 4 lap, 6 air, 8 chip, 9 pickup,
// 10 mega, 12 Turbo Up, 13 Shield Low, 14 Opponent Shielded, 15 Hit by.
#pragma once
#include <array>
#include <cstddef>
#include <vector>

struct PointsPopup
{
    int kind, points, pilot;   // pilot: the name "Killed" (kind 2) and "Hit by" (kind 15) show
};

class RacePoints
{
  public:
    static constexpr int kKinds = 16;
    static constexpr float kPopupSeconds = 0.8f;   // 0x4cc30: how long the shown popup lasts

    void resetHud();         // 0x49360: the target, the shown score and the popups, with the HUD
    void clearBreakdown();   // the points per kind, at the race's start

    // 0x4cb00: at most 32 popups, 0 points only for kinds 12..15, the same kind adds to its entry (re-timed to half
    // way when it is the one showing, its pilot kept); the first entry of an empty queue starts now
    void award(int points, int kind, int pilot, float now);
    // one 60 Hz step: the shown popup goes after 0.8 s (the next one starts then), the shown score climbs by
    // max(gap / 4, 100), never past the target
    void step(float now);

    int target() const;
    int shown() const;
    int ofKind(int kind) const;

    bool hasPopup() const;
    const PointsPopup &popup() const;   // the one showing (hasPopup)
    float popupShownFor(float now) const;
    size_t queuedPopups() const;

  private:
    int target_ = 0, shown_ = 0;
    std::array<int, kKinds> byKind_{};
    std::vector<PointsPopup> popups_;
    float popupStart_ = 0.f;
};
