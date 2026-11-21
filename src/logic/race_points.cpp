// Argon: the points a race earns as the HUD keeps them (race_points.hpp).
#include "logic/race_points.hpp"

#include <algorithm>

namespace
{
constexpr size_t kMaxPopups = 32;
constexpr int kFirstMessageKind = 12, kLastMessageKind = 15;   // shown without points
}  // namespace

void RacePoints::resetHud()
{
    target_ = shown_ = 0;
    popups_.clear();
}

void RacePoints::clearBreakdown()
{
    byKind_.fill(0);
}

void RacePoints::award(int points, int kind, int pilot, float now)
{
    if (points > 0)
    {
        target_ += points;
    }
    if (points > 0 && kind >= 0 && kind < kKinds)
    {
        byKind_[size_t(kind)] += points;
    }
    const bool message = kind >= kFirstMessageKind && kind <= kLastMessageKind;
    if (popups_.size() >= kMaxPopups || (points == 0 && !message))
    {
        return;
    }
    for (size_t index = 0; index < popups_.size(); ++index)
    {
        if (popups_[index].kind == kind)
        {
            popups_[index].points += points;
            if (index == 0 && now - popupStart_ > kPopupSeconds / 2.f)
            {
                popupStart_ = now - kPopupSeconds / 2.f;
            }
            return;
        }
    }
    if (popups_.empty())
    {
        popupStart_ = now;
    }
    popups_.push_back({kind, points, pilot});
}

void RacePoints::step(float now)
{
    if (!popups_.empty() && now - popupStart_ > kPopupSeconds)
    {
        popups_.erase(popups_.begin());
        popupStart_ += kPopupSeconds;
    }
    if (shown_ != target_)
    {
        const int climb = std::max((target_ - shown_) >> 2, 100);
        shown_ = std::min(shown_ + climb, target_);
    }
}

int RacePoints::target() const
{
    return target_;
}

int RacePoints::shown() const
{
    return shown_;
}

int RacePoints::ofKind(int kind) const
{
    return kind >= 0 && kind < kKinds ? byKind_[size_t(kind)] : 0;
}

bool RacePoints::hasPopup() const
{
    return !popups_.empty();
}

const PointsPopup &RacePoints::popup() const
{
    return popups_.front();
}

float RacePoints::popupShownFor(float now) const
{
    return now - popupStart_;
}

size_t RacePoints::queuedPopups() const
{
    return popups_.size();
}
