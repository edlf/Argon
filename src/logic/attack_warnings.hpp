// Argon: "ATTACK WARNING" (docs\weapons\hud.md "Attack warning"): a craft in danger this step has its weapon object's
// +0xb5 set by the weapons' update; 0x6bd70 copies it into huddata +0x108 bit 0x20 and clears it, and the power-up
// panel (0x4d720) shows race string 0x51f while the bit is set.
#pragma once
#include <array>
#include <cmath>

#include <glm/glm.hpp>

namespace argon
{

constexpr float kProjectileWarningRange = 300.f;   // 0x66eaa: distance squared under 90000 (0x1a3a08)
constexpr float kPathWarningRange = 200.f;         // 0x68559 / 0x6857a (0x1a35a0)

// a point against a path: how far it lies off the path's line and how far along it its foot is
struct PathOffset
{
    float distance = 0.f, along = 0.f, length = 0.f;
};

inline PathOffset pathOffset(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &point)
{
    const glm::vec3 path = to - from, offset = point - from;
    const float lengthSquared = glm::dot(path, path);
    if (lengthSquared <= 0.f)
    {
        return PathOffset{glm::length(offset), 0.f, 0.f};
    }
    const float along = glm::dot(offset, path) / lengthSquared, length = std::sqrt(lengthSquared);
    return PathOffset{glm::length(offset - path * along), along * length, length};
}

// 0x683b0 (0x68547..0x68591): a craft inside the hit radius, ahead of the path's start and at most 200 m past its end
inline bool pathWarns(const PathOffset &offset, float hitRadius)
{
    return offset.distance <= hitRadius && offset.distance < kPathWarningRange && offset.along > 0.f && offset.along < offset.length + kPathWarningRange;
}

// the per-craft flags of one 60 Hz step; the HUD shows the last finished step's
class AttackWarnings
{
  public:
    void beginStep()
    {
        building_.fill(false);
    }
    void warn(int slot)
    {
        if (slot >= 0 && slot < int(building_.size()))
        {
            building_[size_t(slot)] = true;
        }
    }
    void endStep()
    {
        shown_ = building_;
    }
    void reset()
    {
        building_.fill(false);
        shown_.fill(false);
    }
    bool shown(int slot) const
    {
        return slot >= 0 && slot < int(shown_.size()) && shown_[size_t(slot)];
    }

  private:
    std::array<bool, 16> building_{}, shown_{};
};

}  // namespace argon
