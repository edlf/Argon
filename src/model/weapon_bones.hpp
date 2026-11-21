// Argon: the ship parts that only show while a weapon or shield is active (0x506e0, 0xb97f0).
#pragma once
#include "model/cmm.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace argon
{

// The bones named by data\weapon.nwp (the weapon definitions: ROCKET, FIREBOMB, AVALANCHE, MAGMA, ...) and the craft
// loader's SHIELD / TRAIL bones (0x506e0), with their child bones (the game switches whole subtrees, 0xb97f0). A bone
// matches when its name without trailing digits / underscores ends with one of those names (some are clipped in the
// file), or when SPECIAL, revol or SHIELD is anywhere in it (1SHIELD_TARA0, revol3).
class WeaponBones
{
  public:
    static WeaponBones fromWeaponFile(const std::vector<uint8_t> &weaponFile);
    bool marks(const std::vector<CmmBone> &bones, int bone) const;

  private:
    bool namesWeapon(const std::string &boneName) const;
    std::vector<std::string> names_;
};

}  // namespace argon
