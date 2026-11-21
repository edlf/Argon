// Argon: the integration (0xcedd0), the hull's contacts and craft-pair knocks (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include "craft/row_major.hpp"

namespace argon
{

// ------------------------------------------------------------------ integration (0xcedd0)
void GameCraft::integrate(float substep)
{
    if (substep > 0.1f)
    {
        substep = 0.1f;
    }
    State &current = currentState();
    State &next = nextState();
    // rotation: R' = R + h * star(Iw L) * R, Iw = R Ib^-1 R^T
    float Ib[9] = {IbInv_.x, 0, 0, 0, IbInv_.y, 0, 0, 0, IbInv_.z};
    float RI[9], Rt[9], Iw[9];
    mul(current.rotation, Ib, RI);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            Rt[i * 3 + j] = current.rotation[j * 3 + i];
        }
    }
    mul(RI, Rt, Iw);
    glm::vec3 angularVelocity = mulv(Iw, current.angularMomentum);
    float skew[9] = {0, -2.f * angularVelocity.z, angularVelocity.y, 2.f * angularVelocity.z, 0, -angularVelocity.x, -angularVelocity.y, angularVelocity.x, 0};   // 0xceb30 (z doubled, as shipped)
    float skewRotation[9];
    mul(skew, current.rotation, skewRotation);
    for (int i = 0; i < 9; ++i)
    {
        next.rotation[i] = current.rotation[i] + substep * skewRotation[i];
    }
    // 0xceba0: normalise columns; 0xeb9f0: rebuild one row by cross products, cycling
    for (int c = 0; c < 3; ++c)
    {
        float length = std::sqrt(next.rotation[c] * next.rotation[c] + next.rotation[3 + c] * next.rotation[3 + c] + next.rotation[6 + c] * next.rotation[6 + c]);
        if (length > 1e-6f)
        {
            next.rotation[c] /= length;
            next.rotation[3 + c] /= length;
            next.rotation[6 + c] /= length;
        }
    }
    float *rotation = next.rotation;
    orthoCycle_ = (orthoCycle_ + 1) % 3;
    if (orthoCycle_ == 0)
    {
        rotation[0] = rotation[8] * rotation[4] - rotation[7] * rotation[5];
        rotation[1] = rotation[6] * rotation[5] - rotation[8] * rotation[3];
        rotation[2] = rotation[7] * rotation[3] - rotation[6] * rotation[4];
        rotation[3] = rotation[2] * rotation[7] - rotation[1] * rotation[8];
        rotation[4] = rotation[8] * rotation[0] - rotation[6] * rotation[2];
        rotation[5] = rotation[6] * rotation[1] - rotation[7] * rotation[0];
    }
    else if (orthoCycle_ == 1)
    {
        rotation[3] = rotation[7] * rotation[2] - rotation[1] * rotation[8];
        rotation[4] = rotation[8] * rotation[0] - rotation[6] * rotation[2];
        rotation[5] = rotation[6] * rotation[1] - rotation[7] * rotation[0];
        rotation[6] = rotation[5] * rotation[1] - rotation[2] * rotation[4];
        rotation[7] = rotation[2] * rotation[3] - rotation[5] * rotation[0];
        rotation[8] = rotation[4] * rotation[0] - rotation[1] * rotation[3];
    }
    else
    {
        rotation[6] = rotation[1] * rotation[5] - rotation[2] * rotation[4];
        rotation[7] = rotation[2] * rotation[3] - rotation[5] * rotation[0];
        rotation[8] = rotation[4] * rotation[0] - rotation[1] * rotation[3];
        rotation[0] = rotation[8] * rotation[4] - rotation[7] * rotation[5];
        rotation[1] = rotation[6] * rotation[5] - rotation[8] * rotation[3];
        rotation[2] = rotation[7] * rotation[3] - rotation[6] * rotation[4];
    }
    bool bad = false;
    for (int i = 0; i < 9; ++i)
    {
        if (!std::isfinite(rotation[i]))
        {
            bad = true;
        }
        rotation[i] = std::clamp(rotation[i], -1.f, 1.f);
    }
    if (bad)
    {
        std::memset(rotation, 0, 36);
        rotation[0] = rotation[4] = rotation[8] = 1;
    }
    // linear / angular (explicit position from the old velocity)
    next.position = current.position + substep * current.velocity;
    next.momentum = current.momentum + substep * current.force;
    next.velocity = next.momentum * invMass_;
    next.angularMomentum = current.angularMomentum + substep * current.torque;
    next.angularVelocity = IbInv_ * next.angularMomentum;                 // stored as Ib^-1 * L, like the game
    next.force = current.force;
    next.torque = current.torque;
}

// physics_substep 0xcffb0 after the box points: the new state's hull contacts (0xd3c70), then each in turn: a reset
// triangle's (contact type 2, 0xd4256) only asks for the reset (0xd0273), the others go to the response 0xd5140
void GameCraft::collide(const SimWorld &world)
{
    const State &state = nextState();
    const std::vector<hull::Contact> found = hull::boxContacts(placedBox(state), world.spanTriangles(hull::queryBounds(rotationOf(state.rotation), state.position, box_)));
    ContactPass pass;
    pass.boundVelocity = state.velocity;   // contact_bind_state 0xd05c0
    for (const hull::Contact &contact : found)
    {
        const TriangleSurface surface = world.surfaceOf(contact.triangle);
        if (surface.resetsCraft)
        {
            resetRequested_ = true;
            continue;
        }
        respond(contact, surface, pass);
    }
    jolt_.addSubstep(pass.jolt);   // 0xd0a30 at the substep's end (0xcff92)
}

hull::PlacedBox GameCraft::placedBox(const State &state) const
{
    return hull::placeBox(rotationOf(state.rotation), state.position, box_);
}

// 0xd5140 (n into the surface): only a contact with the world triangle's normal moves the craft (physics_substep
// passes 0 as the second argument); the lever is taken from the position as this contact finds it
void GameCraft::respond(const hull::Contact &contact, const TriangleSurface &surface, ContactPass &pass)
{
    const glm::vec3 &vcopy = pass.boundVelocity;
    if (contact.source != hull::NormalSource::WorldTriangle || vcopy == glm::vec3(0.f))
    {
        return;
    }
    State &state = nextState();
    const glm::vec3 &normal = contact.normal;
    const glm::vec3 lever = contact.point - state.position;
    const glm::vec3 up = col(state.rotation, 1);
    float alignment = glm::dot(normal, vcopy * (1.f / std::sqrt(glm::dot(vcopy, vcopy))));
    float pn = mass_ * glm::dot(state.velocity, normal);
    if (alignment > 0)
    {
        state.position -= 0.005f * alignment * normal;
    }
    if (pn > 0)
    {
        const glm::vec3 before = state.velocity;
        const glm::vec3 momentumBefore = state.momentum;
        state.momentum -= glm::vec3(1.2f, 1.03f, 1.2f) * pn * normal;
        if (glm::dot(normal, up) > -0.5f)
        {
            float len = glm::length(state.momentum);
            if (len > 1e-6f)
            {
                glm::vec3 ph = state.momentum / len;
                float fr = (-glm::dot(ph, normal) * 0.8f + 0.2f) * std::min(len, 70.f);
                state.momentum -= ph * fr * 0.1f;
            }
        }
        state.velocity = state.momentum * invMass_;
        state.position -= 0.005f * pn * normal;
        noteWorldContact(state.velocity - before, alignment, surface.type);
        pass.jolt.add(state.momentum - momentumBefore, alignment);
        ++wallHits_;
        hitNow_ += pn;                                 // the contact's impulse feeds the rumble (0x442e98)
    }
    float pc = mass_ * glm::dot(vcopy, normal);
    if (pc > 0)
    {
        glm::vec3 spin = glm::cross(lever, normal) * pc * 0.013f * 0.33f;
        if (glm::length(spin) > 1.f)
        {
            spin = glm::normalize(spin);               // 0xd55d2: at most 1 a contact (0x58360)
        }
        state.angularMomentum -= spin;
    }
}

// physics_substep 0xcffb0: each response adds its velocity change and normal . direction to the craft's record
void GameCraft::noteWorldContact(const glm::vec3 &velocityChange, float normalDot, int surface)
{
    sound::CraftContact &contact = soundContacts_.world;
    ++contact.count;
    contact.velocityChange += velocityChange;
    contact.normalDotSum += normalDot;
    contact.surface = surface;
}

// craft_water 0x61340 tests each corner under water against the craft's vertical speed; the first one counts
void GameCraft::noteSplash(float verticalSpeed)
{
    if (soundContacts_.splashed)
    {
        return;
    }
    soundContacts_.splashed = true;
    soundContacts_.splashSpeed = verticalSpeed;
}

void GameCraft::boxBounds(glm::vec3 &lo, glm::vec3 &hi) const
{
    lo = hi = shape_[current_][0];
    for (auto &corner : shape_[current_])
    {
        lo = glm::min(lo, corner);
        hi = glm::max(hi, corner);
    }
}

// 0xd4410: a pair whose hull boxes overlap on all three axes (sweep and prune 0xd4d30) gets one contact: point =
// between the box centres, normal = normalize(centre b - centre a). 0xd5140 (two bodies): J = m_a (v_a - v_b) . n;
// if J > 0: J += 5, P_a -= 0.7 J n, v_a = P_a / m_a, P_b += 0.7 J n and v_b = P_a / m_a (as shipped).
bool GameCraft::collidePair(GameCraft &first, GameCraft &second)
{
    glm::vec3 alo, ahi, blo, bhi;
    first.boxBounds(alo, ahi);
    second.boxBounds(blo, bhi);
    if (ahi.x < blo.x || bhi.x < alo.x || ahi.y < blo.y || bhi.y < alo.y || ahi.z < blo.z || bhi.z < alo.z)
    {
        return false;
    }
    glm::vec3 ca = (alo + ahi) * 0.5f, cb = (blo + bhi) * 0.5f, d = cb - ca;
    if (d == glm::vec3(0))
    {
        d.x += 0.1f;
    }
    glm::vec3 normal = glm::normalize(d);
    State &sa = first.currentState();
    State &sb = second.currentState();
    float impulse = glm::dot(sa.velocity - sb.velocity, normal) * first.mass_;
    if (impulse <= 0.f)
    {
        return true;
    }
    impulse += 5.f;
    const glm::vec3 before = sa.velocity;
    sa.momentum -= 0.7f * impulse * normal;
    sa.velocity = sa.momentum * first.invMass_;
    first.soundContacts_.pairChange = std::max(first.soundContacts_.pairChange, glm::length(sa.velocity - before));
    sb.momentum += 0.7f * impulse * normal;
    sb.velocity = sa.velocity;
    first.hitNow_ += impulse;   // craft contacts rumble both pads (0x4479dc list in craft_update)
    second.hitNow_ += impulse;
    return true;
}

} // namespace argon
