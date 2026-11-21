// Argon: tests for the weapon and shield bones a prop hides (model\weapon_bones.hpp: 0x506e0, 0xb97f0).
// Run with test.bat / test.sh.
#include "model/weapon_bones.hpp"
#include "check.hpp"

#include <cstdio>
#include <string>

namespace
{
using argon::test::checks;
using argon::test::failures;

std::vector<uint8_t> weaponFile(const std::string &names)
{
    return std::vector<uint8_t>(names.begin(), names.end());
}

argon::CmmBone bone(const std::string &name, int parent)
{
    argon::CmmBone b;
    b.name = name;
    b.parent = parent;
    return b;
}

void boneNamedInTheFileIsMarked()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile(std::string("ROCKET") + '\0'));
    const std::vector<argon::CmmBone> bones = {bone("ROCKET", -1)};

    CHECK(weapons.marks(bones, 0));
}

void trailingNumberIsIgnored()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile(std::string("FIREBOMB") + '\0'));
    const std::vector<argon::CmmBone> bones = {bone("FIREBOMB_2", -1)};

    CHECK(weapons.marks(bones, 0));
}

void childOfAWeaponBoneIsMarked()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile(std::string("MAGMA") + '\0'));
    const std::vector<argon::CmmBone> bones = {bone("MAGMA1", -1), bone("NOZZLE", 0)};

    CHECK(weapons.marks(bones, 1));
}

void shortNameInTheFileIsSkipped()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile(std::string("GUN") + '\0'));
    const std::vector<argon::CmmBone> bones = {bone("GUN", -1)};

    CHECK(!weapons.marks(bones, 0));
}

void nameNotEndedByZeroIsSkipped()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile("BLASTER "));
    const std::vector<argon::CmmBone> bones = {bone("BLASTER", -1)};

    CHECK(!weapons.marks(bones, 0));
}

void shieldAnywhereInTheNameIsMarked()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile({});
    const std::vector<argon::CmmBone> bones = {bone("1SHIELD_TARA0", -1)};

    CHECK(weapons.marks(bones, 0));
}

void loaderNameIsMarkedWithoutTheFile()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile({});
    const std::vector<argon::CmmBone> bones = {bone("LEFTTRAIL", -1)};

    CHECK(weapons.marks(bones, 0));
}

void plainBoneIsNotMarked()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile(weaponFile(std::string("ROCKET") + '\0'));
    const std::vector<argon::CmmBone> bones = {bone("BODY", -1)};

    CHECK(!weapons.marks(bones, 0));
}

void noBoneIsNotMarked()
{
    const argon::WeaponBones weapons = argon::WeaponBones::fromWeaponFile({});
    const std::vector<argon::CmmBone> bones = {bone("SHIELD", -1)};

    CHECK(!weapons.marks(bones, -1));
}

}  // namespace

int main()
{
    boneNamedInTheFileIsMarked();
    trailingNumberIsIgnored();
    childOfAWeaponBoneIsMarked();
    shortNameInTheFileIsSkipped();
    nameNotEndedByZeroIsSkipped();
    shieldAnywhereInTheNameIsMarked();
    loaderNameIsMarkedWithoutTheFile();
    plainBoneIsNotMarked();
    noBoneIsNotMarked();
    std::printf("weapon_bones_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
