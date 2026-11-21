// Argon: tests for the sprite pool's quads (sprite_pool.hpp: 0xbd670's corners and uvs, 0x9c540's angle slots,
// quadsprite.xvu's turn, 0xbd8f0's bases). Run with test.bat / test.sh.
#include "render/sprite_pool.hpp"
#include "check.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

const float kQuarterTurn = 1.5707963f;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

bool near(const glm::vec2 &a, const glm::vec2 &b)
{
    return glm::length(a - b) < 1e-6f;
}

glm::mat4 lookingDownMinusZ()
{
    return glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
}

// looking down -z from the origin: the viewer's right is +x, so the game's view x axis is -x
void cameraBasisIsTheGamesViewAxes()
{
    const SpriteBasis b = spriteBasis(SpriteFacing::Camera, lookingDownMinusZ());
    CHECK(near(b.x, {-1, 0, 0}) && near(b.y, {0, 1, 0}) && near(b.z, {0, 0, -1}));
}

void uprightBasisStaysLevel()
{
    const glm::mat4 view = glm::lookAt(glm::vec3(0), glm::vec3(0, -1, -1), glm::vec3(0, 1, 0));
    const SpriteBasis b = spriteBasis(SpriteFacing::Upright, view);
    CHECK(near(b.x, {-1, 0, 0}) && near(b.y, {0, 1, 0}) && near(b.z, {0, 0, -1}));
}

void worldBasisIsIdentity()
{
    const glm::mat4 view = glm::lookAt(glm::vec3(5), glm::vec3(0), glm::vec3(0, 1, 0));
    const SpriteBasis b = spriteBasis(SpriteFacing::World, view);
    CHECK(near(b.x, {1, 0, 0}) && near(b.y, {0, 1, 0}) && near(b.z, {0, 0, 1}));
}

// 0xbd670 writes (sin, cos) of sprite.dat's 90 degrees: (1, 0), the default turn
void defaultTurnLeavesTheOffset()
{
    CHECK(near(spriteCornerOffset({1, 2, 3}, SpriteTurn{}, SpriteBasis{}), {1, 2, 3}));
}

void zeroAngleIsTheDefaultTurn()
{
    const SpriteTurn t = SpriteTurn::ofAngle(0.f);
    CHECK(t.first == 1.f && t.second == 0.f);
}

// 0x9c540: (cos, -sin); the shader's (x first + y second, y first - x second) turns x towards y
void quarterTurnTakesXToY()
{
    CHECK(near(spriteCornerOffset({1, 0, 0}, SpriteTurn::ofAngle(kQuarterTurn), SpriteBasis{}), {0, 1, 0}));
}

// 0xbd670's own (sin, cos): its 90 degrees leaves the offsets as they are (sprite.dat)
void allocatedQuarterTurnIsTheDefaultTurn()
{
    const SpriteTurn t = SpriteTurn::ofAllocation(kQuarterTurn);
    CHECK(near(spriteCornerOffset({1, 2, 0}, t, SpriteBasis{}), {1, 2, 0}));
}

// at 0 degrees the slots are (0, 1): x goes to -y
void allocatedZeroAngleTakesXToMinusY()
{
    CHECK(near(spriteCornerOffset({1, 0, 0}, SpriteTurn::ofAllocation(0.f), SpriteBasis{}), {0, -1, 0}));
}

void turnKeepsTheOffsetsZ()
{
    CHECK(near(spriteCornerOffset({0, 0, 5}, SpriteTurn::ofAngle(kQuarterTurn), SpriteBasis{}), {0, 0, 5}));
}

// the game's x axis points to the screen's left: a corner on the screen's right goes down a quarter turn later
void cameraFacingSpinIsClockwiseOnScreen()
{
    const SpriteBasis b = spriteBasis(SpriteFacing::Camera, lookingDownMinusZ());
    CHECK(near(spriteCornerOffset({-1, 0, 0}, SpriteTurn{}, b), {1, 0, 0}));
    CHECK(near(spriteCornerOffset({-1, 0, 0}, SpriteTurn::ofAngle(kQuarterTurn), b), {0, -1, 0}));
}

void squareCornersAreTheAllocatorsOrder()
{
    CHECK(near(squareCornerOffset(2.f, 0), {-2, -2, 0}));
    CHECK(near(squareCornerOffset(2.f, 1), {2, -2, 0}));
    CHECK(near(squareCornerOffset(2.f, 2), {-2, 2, 0}));
    CHECK(near(squareCornerOffset(2.f, 3), {2, 2, 0}));
}

void standingCornersRiseFromThePosition()
{
    CHECK(near(standingCornerOffset(1.5f, 4.f, 0), {-1.5f, 4, 0}));
    CHECK(near(standingCornerOffset(1.5f, 4.f, 1), {1.5f, 4, 0}));
    CHECK(near(standingCornerOffset(1.5f, 4.f, 2), {-1.5f, 0, 0}));
    CHECK(near(standingCornerOffset(1.5f, 4.f, 3), {1.5f, 0, 0}));
}

// the puffs' cell (0, 384), 128 px: centre (64 / 512, 448 / 1024) -+ 0.97 x (64 / 512, 64 / 1024)
void cellUvIsInsetAroundItsCentre()
{
    const SpriteCellUv cell = SpriteCellUv::of({0.f, 384.f}, 128.f);
    CHECK(near(cell.low, {0.125f - 0.12125f, 0.4375f - 0.060625f}));
    CHECK(near(cell.high, {0.125f + 0.12125f, 0.4375f + 0.060625f}));
}

void cellCornersFollowTheVertices()
{
    const SpriteCellUv cell{{0.1f, 0.2f}, {0.3f, 0.4f}};
    CHECK(near(cell.at(0), {0.1f, 0.2f}));
    CHECK(near(cell.at(1), {0.3f, 0.2f}));
    CHECK(near(cell.at(2), {0.1f, 0.4f}));
    CHECK(near(cell.at(3), {0.3f, 0.4f}));
}

void trianglesAreV0V1V2ThenV1V2V3()
{
    const int expected[kSpriteTriangleVertices] = {0, 1, 2, 1, 2, 3};
    bool same = true;
    for (int k = 0; k < kSpriteTriangleVertices; ++k)
    {
        same = same && kSpriteTriangleCorners[k] == expected[k];
    }
    CHECK(same);
}
}  // namespace

int main()
{
    cameraBasisIsTheGamesViewAxes();
    uprightBasisStaysLevel();
    worldBasisIsIdentity();
    defaultTurnLeavesTheOffset();
    zeroAngleIsTheDefaultTurn();
    quarterTurnTakesXToY();
    allocatedQuarterTurnIsTheDefaultTurn();
    allocatedZeroAngleTakesXToMinusY();
    turnKeepsTheOffsetsZ();
    cameraFacingSpinIsClockwiseOnScreen();
    squareCornersAreTheAllocatorsOrder();
    standingCornersRiseFromThePosition();
    cellUvIsInsetAroundItsCentre();
    cellCornersFollowTheVertices();
    trianglesAreV0V1V2ThenV1V2V3();
    std::printf("sprite_pool_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
