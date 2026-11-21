// Port of Quantum Redshift's own craft physics and AI pilot (default.xbe), for the viewer's AI drive.
//
// Every function below names the XBE routine it follows (see docs\physics\README.md for the
// decoding). Field comments give the original craft-object offsets. What is NOT ported:
// weapons/shields/turbo pickups, opponents (overtaking offsets, catch-up). The hull contacts are 0xd3c70
// (logic/hull_contacts.hpp), the water's depth with its waves is 0xce6b0 (logic/water_waves.hpp).
#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "track/collision_surface.hpp"
#include "craft/drive.hpp"
#include "logic/ai_routes.hpp"
#include "logic/blast_push.hpp"
#include "logic/camera_shake.hpp"
#include "logic/hover_dust.hpp"
#include "logic/hull_contacts.hpp"
#include "logic/sound_rules.hpp"
#include "logic/water_waves.hpp"
#include "logic/weapon_effects.hpp"
#include "track/trackdata.hpp"

namespace argon
{

// config\character_stats.txt (parser 0x72390): 16 pilot columns
class CraftStats
{
  public:
    void load(const std::string &path);   // throws std::runtime_error when it cannot be read or holds no values
    float get(const char *key, int pilot) const;
    float gear(int set, int gearIndex, int pilot) const
    {
        return gears_[set][gearIndex][pilot];
    }   // set = speed class 0..4
    bool loaded() const
    {
        return !v_.empty();
    }

  private:
    std::map<std::string, std::vector<float>> v_;
    float gears_[5][10][16]{};
};

// data\craft folder -> character_stats column (Pharaoh's folder is "zafir")
int pilotIndex(const std::string &folder);
const char *pilotName(int index);
const char *pilotFolder(int index);   // data\craft folder name

struct SimWorld;

// AI node graph at runtime (CAIMap, nodes 0x54 bytes at *0x1b9bc8)
struct AiGraph
{
    struct Node
    {
        glm::vec3 pos, down;
        std::vector<int> fwd;
        uint32_t flags = 0;
    };
    enum : uint32_t
    {
        First = 2,
        Restart = 4,
        EnvInside = 0x80,   // env_inside (0xe2e3e)
        TurboGoodSlow = 0x2000,   // turbo_good_<speed>: 0x2000 << speed class
        CornerSlow = 0x40000,
        Hairpin = 0x80000,
        Straight = 0x100000
    };
    std::vector<Node> nodes;
    int first = 0;
    int last = 0;                // the main loop's node before the first one
    // route distance of each node from the first one: the main loop's, a side branch's nodes evenly between its fork
    // and its merge (0xe4460); -1 on nodes no route reaches
    std::vector<float> dist;
    float length = 0;
    std::vector<airoutes::RouteNode> routes;   // the route table (ai_route_table.cpp), empty until buildRoutes
    void build(const TrackData &td);
    // the route table for a race: the branches' surfaces under the world, their pickups at the race's speed class
    void buildRoutes(const SimWorld &world, const std::vector<Pickup> &pickups, int speedClass);
    int next(int node) const
    {
        return nodes[node].fwd.empty() ? node : nodes[node].fwd[0];
    }

  private:
    void measureMainLoop();
    void spreadSideBranches();
    void spreadSideBranch(int fork, int branch);
};

// track collision (the meshes the game builds collision from) + water.dat boxes and their waves
struct SimWorld
{
    const CollisionGrid *grid = nullptr;
    const std::vector<TriangleSurface> *surf = nullptr;   // per collision triangle
    std::vector<WaterBox> water;
    WaterWaves waves;
    float clock = 0.f;   // the game clock the waves run on (0x50e430 + 0x50e434, from the race setup)
    bool segment(const glm::vec3 &from, const glm::vec3 &to, glm::vec3 &hit, glm::vec3 &normal, TriangleSurface &surface) const;
    // 0xd3770: the triangles whose spans meet the query, in span order (mesh, then triangle)
    std::vector<hull::WorldTriangle> spanTriangles(const hull::Bounds &query) const;
    TriangleSurface surfaceOf(uint32_t triangle) const;
    bool waterDepth(const glm::vec3 &p, float &depth) const;   // 0xce6b0: p.y - (box top + wave)
};

class GameCraft
{
  public:
    struct State
    {          // one of the two 0x78-byte blocks at craft +0x2c
        glm::vec3 position{0};          // +0x00
        float rotation[9]{1, 0, 0, 0, 1, 0, 0, 0, 1};   // +0x0c row major; columns = right, up, forward
        glm::vec3 angularVelocity{0};   // +0x30 (Ib^-1 * angularMomentum, as the game stores it)
        glm::vec3 velocity{0};          // +0x3c
        glm::vec3 momentum{0};          // +0x48
        glm::vec3 angularMomentum{0};   // +0x54
        glm::vec3 force{0};             // +0x60
        glm::vec3 torque{0};            // +0x6c
    };

    // who flies the craft (the game's two craft classes, docs\physics\README.md "Classes"): the player's pad (CPlayer,
    // vtable 0x19f3e8) or the AI (drone, vtable 0x1a1c10). The game fixes the class when it builds the craft; the
    // viewer can switch while racing ("Drive yourself", AI drive), so the craft keeps a controller object instead.
    // Pad: the viewer's own pad, all 0..1; steer -1 left .. +1 right. An empty pad holds the craft (the countdown).
    struct Pad
    {
        float throttle = 0, brake = 0, steer = 0, airLeft = 0, airRight = 0, dive = 0, climb = 0;
        bool turbo = false;
        bool reset = false;   // BACK (pad bit 0x200): put back on the track (player_pad_input 0x71023)
    };
    void flyByPad(const Pad &p);
    void flyByPad()   // an empty pad: the craft held (the countdown)
    {
        flyByPad(Pad{});
    }
    void flyByAi();
    // a replay's pose (craft_replay_inputs 0x61c60 -> 0xd06f0): both state blocks placed, the physics not run, the
    // cockpit view without the jolt and the shake (0x5ba10 skipped); pose columns right, up, forward, position (as matrix())
    void placeFromReplay(const glm::mat4 &pose, const glm::vec3 &velocity);
    // a replayed craft's lap line and lap clock from its placed pose, as frame() runs them for a flying one
    void followReplay(const AiGraph &graph, float dt)
    {
        trackProgress(graph, dt);
    }
    // craft_replay_inputs 0x61c60: the recorded control bits as the slots (0 / 100) and the brake, which the flaps and
    // the jet read
    void setReplayControls(const std::array<float, 13> &slots)
    {
        std::copy(slots.begin(), slots.end(), ctrl_);
        brake_ = slots[9];
    }
    bool flownByPad() const;
    // the route at the AI nodes' forks (0xe1370, logic\ai_routes.hpp): the list the craft follows this frame (its
    // look-ahead and its node along the route), or each node's first link; the seed is the strategy's rand() (+0x54c)
    void followRoute(airoutes::RouteList list)
    {
        routeList_ = list;
    }
    void followFirstLinks()
    {
        routeList_.reset();
    }
    void setRouteSeed(int seed)
    {
        routeSeed_ = seed;
    }
    std::optional<airoutes::RouteList> route() const
    {
        return routeList_;
    }
    // the last fork the craft's node passed and the branch it took there; count = forks passed so far
    struct ForkPassed
    {
        int node = -1, branch = -1, count = 0;
    };
    const ForkPassed &lastFork() const
    {
        return lastFork_;
    }
    // catch-up (0x6d650), set before frame(): engine gain + gainAdd, engine power x powerScale
    void setCatchUp(float gainAdd, float powerScale)
    {
        catchGain_ = gainAdd;
        catchPower_ = powerScale;
    }
    float catchUpGain() const
    {
        return catchGain_;
    }
    float catchUpPower() const
    {
        return catchPower_;
    }
    // overtaking (0xe4be0): gap in % of a lap to the craft one place ahead (< 5 to act), side 0 / 1
    void setOvertake(bool on, float gap, int side)
    {
        overtake_ = on;
        overGap_ = gap;
        overSide_ = side;
    }
    // craft vs craft (0xcf920 -> 0xd4d30 / 0xd4410 -> 0xd5140 two-body branch), once per frame after both moved
    static bool collidePair(GameCraft &first, GameCraft &second);
    void boxBounds(glm::vec3 &lo, glm::vec3 &hi) const;
    void init(const CraftStats &stats, int pilot, int speedClass, float aiPowerPct, const glm::mat4 &start, const AiGraph &graph, int startNode,
              const SimWorld &world);
    // one game frame: AI pilot + engine + physics world step (substepped like 0xcf630)
    void frame(float dt, const SimWorld &world, const AiGraph &graph);

    // --- read-outs for the viewer ---
    glm::mat4 matrix() const;
    glm::vec3 position() const
    {
        return currentState().position;
    }
    float speedKmh() const
    {
        return speed_ * 3.6f;
    }        // |v|
    float forwardSpeed() const
    {
        return fwdSpeed_;
    }        // +0x990 (m/s)
    float revs() const
    {
        return revs_;
    }                    // +0x940
    int gear() const
    {
        return gear_;
    }                      // +0x93c (-1 while shifting)
    float throttle() const
    {
        return ctrl_[0];
    }             // +0x8f4 (0..100)
    float controlSlot(int slot) const
    {
        return slot >= 0 && slot < 13 ? ctrl_[slot] : 0.f;
    }   // +0x8f4 + 4 slot
    float brakeLevel() const
    {
        return brake_;
    }             // +0x980
    bool hoverTouching() const
    {
        return anyContact_;
    }      // +0x92d: any hover ray touches
    float steerLeft() const
    {
        return steerB_;
    }
    float steerRight() const
    {
        return steerA_;
    }
    float steerError() const
    {
        return ctrl_[12];
    }
    bool onGround() const
    {
        return onGround_;
    }             // +0x92c (3+ hover contacts)
    bool inWater() const
    {
        return inWater_;
    }               // +0x92e
    int wakeCorners() const
    {
        return wakeCorners_;
    }         // effects object +0x90: rear box corners (2, 3) under water
    int surface() const
    {
        return surface_;
    }                // +0x85c 0 track 1 rough 2 loose 3 water 4 slippery
    // the dust the rear hover rays raised this frame (craft_hover 0x5f377..0x5f667, logic\hover_dust.hpp)
    const std::vector<DustPuff> &dustPuffs() const
    {
        return dust_;
    }
    // +0x8ac..0x8af: the mean vertex colour (D3DCOLOR, alpha 0xff) under the hover rays that held the craft this step
    // (kept when none did; 0 before the first); the craft draw lights the ship by it (0x59bc0, render\d3d_lighting.hpp)
    uint32_t groundColour() const
    {
        return groundColour_;
    }
    int node() const
    {
        return node_;
    }
    int pilot() const
    {
        return pilot_;
    }
    int laps() const
    {
        return laps_;
    }
    // every crossing of the lap line, the first one too (it only starts the lap clock), and the lap clock then
    int lineCrossings() const
    {
        return crossings_;
    }
    float crossingLapTime() const
    {
        return crossLapTime_;
    }
    float lapTime() const
    {
        return lapTime_;
    }
    void resetLapTimer()
    {
        lapTime_ = 0;
    }   // 0x63a30 at GO
    float lastLap() const
    {
        return lastLap_;
    }
    float bestLap() const
    {
        return bestLap_;
    }
    int respawns() const
    {
        return respawns_;
    }
    int contacts() const
    {
        return wallHits_;
    }
    int hoverContacts() const
    {
        return contact_[0] + contact_[1] + contact_[2] + contact_[3];
    }
    float hoverDepth(int corner) const
    {
        return depth_[corner];
    }
    float height() const
    {
        return currentState().position.y;
    }
    glm::vec3 target() const
    {
        return target_;
    }            // AI aim point (ai +4)
    // the game's chase cameras: 0 close, 1 mid, 2 far (0x5be00 / 0x5c700), 3 internal / cockpit (0x5aa10)
    struct Tripod
    {
        glm::vec3 pos, look, up;
    };
    Tripod tripod(int view) const;
    float lapProgress(const AiGraph &graph) const;   // 0..1 along the main route
    glm::vec3 velocity() const
    {
        return currentState().velocity;
    }
    glm::vec3 angularMomentum() const
    {
        return currentState().angularMomentum;
    }   // the state block's +0x54, recorded by craft_record_inputs 0x61aa0
    const char *surfaceName() const;
    // this frame's contacts as the sounds read them (0xc2b50, craft_water 0x61340; docs\audio\race-sounds.md "Race sounds")
    struct SoundContacts
    {
        sound::CraftContact world;   // the world contacts, summed as the record 0x442e94 sums them
        float pairChange = 0.f;      // the largest velocity change another craft gave this one (0x4479dc)
        bool splashed = false;       // a corner went under water
        float splashSpeed = 0.f;     // the craft's vertical speed then
    };
    const SoundContacts &soundContacts() const
    {
        return soundContacts_;
    }
    // controller rumble (docs\physics\rumble.md "Rumble"): the two motor speeds 0..65535 the game sends for this craft's
    // pad (player_pad_input 0x70ac0: first = +0x9ea + +0x1138, second = +0x9e8 + +0x1134)
    uint16_t rumbleFirst() const
    {
        return rumA_;
    }
    uint16_t rumbleSecond() const
    {
        return rumB_;
    }
    // turbo (docs\physics\turbo-flaps-cameras.md "Turbo"): +0x8e4 seconds left, +0x9f1 pushing this frame
    float turboSeconds() const
    {
        return turboSecs_;
    }
    bool turboOn() const
    {
        return turboOn_;
    }
    bool turboButton() const
    {
        return turboBtn_;
    }   // +0x9f0
    bool turboHeld() const
    {
        return turboHeld_;
    }   // +0x91f: pad A held (after a let-go)
    void addTurbo(float seconds)
    {
        turboSecs_ = std::clamp(turboSecs_ + seconds, 0.f, 100.f);
    }
    void setTurbo(float seconds)
    {
        turboSecs_ = std::clamp(seconds, 0.f, 100.f);
    }
    // the race is running (not in the countdown, not past the end): turbo may fire (0x52af9e / 0x52afa8)
    void setTurboAllowed(bool on, bool infinite = false)
    {
        turboAllowed_ = on;
        turboInfinite_ = infinite;
    }
    void setSlide(bool on)
    {
        slide_ = on;
    }   // the Slide easter egg (0x52c8bd): the player's sideways grip fixed at 0.1
    void setTurboTest(bool on)
    {
        turboTest_ = on;
    }   // tests: the AI holds the turbo button too
    // 0x58d20: a knocked-out craft is put at `at` (the track's NET_CAGE), upright and at rest, and no longer respawns
    void retire(const glm::vec3 &at);
    // an explosion's push (0x7e410 adds it to +0xa24): the next physics step turns it into the knock +0x8b0, a speed
    // loss and a sideways shove (0x5dee0)
    void pushByBlast(const glm::vec3 &impulse)
    {
        blastPush_ += impulse;
    }
    // 0x5e0ba: the push's speed loss doubles for the drones (craft id > 0) in a Tournament or Quick Race
    void setBlastBrakingDoubled(bool on)
    {
        blastBrakingDoubled_ = on;
    }
    // the internal camera's shake this frame (+0x9cc..+0x9d4, tripod(3) applies it)
    glm::vec3 cameraShake() const
    {
        return shake_.offset();
    }
    // the internal camera's contact jolt (+0x8b4, tripod(3) applies it) along the craft's right, up and forward axes
    glm::vec3 contactJolt() const
    {
        const glm::vec3 jolt = jolt_.vector();
        return {glm::dot(jolt, right_), glm::dot(jolt, up_), glm::dot(jolt, fwd_)};
    }
    float knock() const
    {
        return knock_;
    }   // +0x8b0
    // a beam's first frame on this craft (0x68f5a): the first motor's boost +0x9dc gains 50000
    void addBeamRumble()
    {
        buzzBoost_ += kBeamRumble;
    }
    // a weapon hit's running effects, copied from the craft weapon object each frame (0x5b1d4: +0xa00..+0xa14)
    void setWeaponEffects(const weapons::CraftEffects &effects)
    {
        weaponEffects_ = effects;
    }

  private:
    class Controller;
    class PlayerController;
    class DroneController;
    static const Controller &playerController;
    static const Controller &droneController;
    const Controller *controller_ = &droneController;

    void rumble(float dt);
    float surfaceRumble(float dt);   // 0x5b610's track part: rumble_<surface> x the ground contact
    glm::vec3 shakeDirection();      // 0x5b610's random direction
    float surfaceHertz() const;      // rumblehertz_<surface>
    void decayHitRumble(float dt);   // craft_update 0x71490: +0x1134 / +0x1138
    void takeBlastPush();            // 0x5dee0
    void brakeByBlast(const glm::vec3 &push);
    void noteWorldContact(const glm::vec3 &velocityChange, float normalDot, int surface);
    void noteSplash(float verticalSpeed);                          // 0x5b610 + the hit part of craft_update 0x71490
    void turboInput();                              // player_pad_input 0x70ac0 (+0x91f / +0x920 / +0x9f0)
    void turbo(float substep);                            // craft_turbo 0x5e270
    void droneTurbo(float substep);                       // 0x6f9f0
    bool turboFires() const;
    void drainTurbo(float substep);
    void pushTurbo();
    blast::TurboCooldown turboCooldown_;            // +0x9c4
    float turboSecs_ = 0;                           // +0x8e4
    bool turboHeld_ = false, turboReleased_ = false, turboBtn_ = false;   // +0x91f, +0x920, +0x9f0
    bool turboOn_ = false, turboCap_ = false;       // +0x9f1 pushing, +0x9f3 turbo speed cap
    bool turboAllowed_ = false, turboInfinite_ = false, turboTest_ = false;
    bool slide_ = false;
    float ground_ = 0;                              // +0x9c8 ground contact 0..1
    float hit34_ = 0, hit38_ = 0, hitNow_ = 0;      // +0x1134 / +0x1138 hit rumble, this frame's contact impulses
    static constexpr float kBeamRumble = 50000.f;   // 0x1a3a04
    float buzzBoost_ = 0, knock_ = 0;               // +0x9dc, +0x8b0
    glm::vec3 blastPush_{0.f};                      // +0xa24
    bool blastBrakingDoubled_ = false;
    shake::CameraShake shake_;                      // +0x9cc..+0x9d8
    shake::ContactJolt jolt_;                       // +0x8b4
    uint16_t rumA_ = 0, rumB_ = 0;
    uint32_t randState_ = 1;
    unsigned nextRand();                            // the C runtime's rand() (0xf96ea)
    weapons::CraftEffects weaponEffects_;           // +0xa00..+0xa14
    bool retired_ = false;                          // +0x1145 (knocked out)
    void afflictThrottle();                         // 0x5e475: freeze / engine off / engine jitter on +0x8f4
    void afflictSteering();                         // 0x5fa79: control jitter on +0x970 / +0x974
    // physics pieces (craft vtable slots, see docs\physics\README.md)
    void forces(float substep, const SimWorld &world);       // 0x5d670 order
    void thrust();                                  // 0x5e370
    void hover(const SimWorld &world);                  // 0x5e6d0
    void averageGroundColour(const uint32_t sums[3], int count);   // 0x5ebe6
    // what each hover ray that held the craft hit this substep (surface -1: it held nothing)
    struct HoverHits
    {
        glm::vec3 point[4]{};
        int surface[4] = {-1, -1, -1, -1};
        uint32_t colour[4]{};
    };
    void raiseDust(const HoverHits &hits);   // 0x5f377..0x5f667
    void gravity();                                 // 0x5e600
    void diveClimb();                               // 0x5fe50
    void steering();                                // 0x6fb20 -> 0x5f9d0
    void sideFriction();                            // 0x6f720 (AI factors)
    void playerSideFriction(float substep);               // 0x5ffd0 (player factors, airbrake loosening)
    void aero();                                    // 0x60760
    void angularDamping();                          // 0x5f800
    void noseDrop(float substep);                         // 0x60e40
    void water(const SimWorld &world);                  // 0x61340
    void integrate(float substep);                        // 0xcedd0 + 0xd1620 + clamps
    void collide(const SimWorld &world);                // 0xd3c70 contacts -> 0xd5140 response
    // what the response reads besides the state block: the velocity contact_bind_state 0xd05c0 copied (+0x18) and the
    // substep's contacts for the jolt
    struct ContactPass
    {
        glm::vec3 boundVelocity{0.f};
        shake::SubstepContacts jolt;
    };
    void respond(const hull::Contact &contact, const TriangleSurface &surface, ContactPass &pass);   // 0xd5140
    hull::PlacedBox placedBox(const State &s) const;   // 0xd1620's +0x38 corners and +0x54 normals
    void updateShape(int s);                        // 0xd1620: box points in world space
    void cacheAxes();                               // 0x70430
    // per frame
    void aiPilot(const AiGraph &graph);
    // aiPilot's parts: the corner flags of the tactics points, how far the aim is off the target
    struct AiCorner
    {
        bool slow, straight, hairpin;
    };
    struct AiAimError
    {
        float raw = 0.f, farPoint = 0.f, turned = 0.f;   // to the target, to the far point, the first turned by the yaw
    };
    void fillLookAhead(const AiGraph &graph);
    void placeTargets(const AiGraph &graph);
    void moveTargetsToOvertake();
    float targetBlend() const;
    AiAimError aimError(float blend, const AiCorner &corner) const;
    void steerAtTarget(const AiAimError &error, float blend, const AiCorner &corner);
    void throttleForAim(float err, float blend, const AiCorner &corner);
    void climbOutOfDive();
    void dropPlayerBrakes();                        // the AI zeroes +0x980 and never sets the airbrakes
    void testTurboButton();                         // the AI never presses it (0xe10f0); --turbo-test holds it
    void padPilot();                                // viewer pad -> the same control slots
    void brakeRamp(float dt);                       // 0x71fe0 (player): brake value +0x980
    void brakes();                                  // 0x60450
    void airbrakeRamp(float dt);                    // 0x71bc0 (player): +0x978 / +0x97c
    void airbrakes();                               // 0x60230       // 0xe46a0/0xe4be0/0xe51a0/0xe0810/0xe1080
    void revsGears(float dt);                       // 0x6ed90
    void steerSmooth(float dt);                     // 0x6f0b0 (pilot 0's turn_gain/falloff, as the game does)
    void speedCap();                                // 0x6f900
    void trackProgress(const AiGraph &graph, float dt);
    void respawn(const AiGraph &graph, const SimWorld &world);
    void requestResets(float dt);
    bool upsideDownTooLong(float dt);
    bool stoppedProgressing(float dt);
    void restartProgressCheck();                    // the drone's 2 s "stuck" check from here
    void tripodUpdate(float dt);                   // 0x5c700 sway + camera up smoothing
    void clearCockpitJoltAndShake();               // a replayed craft: tripod(3) without the jolt and the shake
    void dropToGround(const SimWorld &world);   // 0x5cb30: ray +2 .. -20, then 7.5 above the hit
    void settleOnStart(const SimWorld &world);   // 0x5a270: the locator's position, 2 above the ground under it
    glm::vec3 pointAtRadius(const AiGraph &graph, float radius, uint32_t &flags) const;
    int routeNext(const AiGraph &graph, int n) const;   // the next node on the craft's route
    float distanceToSegment(const AiGraph &graph, int node) const;   // to the route segment starting at that node
    void notePassed(const AiGraph &graph, int from, int to);

    float st(const char *key) const
    {
        return stats_->get(key, pilot_);
    }
    float st0(const char *key) const
    {
        return stats_->get(key, 0);
    }   // AI reads +0x1140 = record 0
    float sp(const char *base) const;   // "<base><speed class + 1>"

    const CraftStats *stats_ = nullptr;
    int pilot_ = 0, speedClass_ = 0;
    float aiPower_ = 90.f;
    // the game's two state blocks (craft +0x2c): a step integrates the current one into the next, then swaps
    State states_[2];
    int current_ = 0;
    State &currentState()
    {
        return states_[current_];
    }
    const State &currentState() const
    {
        return states_[current_];
    }
    State &nextState()
    {
        return states_[current_ ^ 1];
    }
    float mass_ = 1, invMass_ = 1;
    glm::vec3 IbInv_{1};                // diagonal of +0x148 (box inertia, 0x6fdc0)
    std::array<glm::vec3, 8> box_{};    // local box corners (0x70340: 4 x 2 x 6)
    std::array<glm::vec3, 8> shape_[2]; // world-space corners per state
    glm::vec3 fwd_{0, 0, 1}, up_{0, 1, 0}, right_{1, 0, 0}, pos_{0};   // +0x250 / +0x25c / +0x268 / +0x27c
    // hover
    bool contact_[4]{};
    float depth_[4]{}, depthSum_ = 0;
    bool anyContact_ = false, onGround_ = false, inWater_ = false;
    SoundContacts soundContacts_;
    std::vector<DustPuff> dust_;   // this frame's, over all substeps
    int surface_ = 0, groundSurf_ = 0, wakeCorners_ = 0;
    uint32_t groundColour_ = 0;
    float airTime_ = 0;
    // controls (+0x8f4 ...): [0] throttle [2] dive [3] climb [5]/[6] steer [7]/[8] airbrake [9] brake [12] error
    float ctrl_[13]{};
    float catchGain_ = 0, catchPower_ = 1;
    bool overtake_ = false;
    float overGap_ = 0;
    int overSide_ = 0;
    float brake_ = 0;                   // +0x980
    float airLevel_ = 0;                // +0xa18 airbrake level the side grip follows
    float air_[2]{};                    // +0x978 / +0x97c smoothed airbrakes (player only)
    Pad pad_;
    float steerA_ = 0, steerB_ = 0;     // +0x970 / +0x974 smoothed steering
    // engine
    float fwdSpeed_ = 0, speed_ = 0;    // +0x990, +0x9a4
    float revs_ = 0, revHist_[10]{}, revAvg_ = 0;
    int gear_ = 0, gearNext_ = 0;
    float shiftTimer_ = 0, gain_ = 1, power_ = 0;   // +0x9b0, +0x9e0, +0x9e4
    float noseTimer_ = 0;               // +0x9b4
    // AI
    std::array<int, 16> look_{};        // look-ahead nodes (ai +0x224..)
    std::optional<airoutes::RouteList> routeList_;
    int routeSeed_ = 0;
    ForkPassed lastFork_;
    glm::vec3 P1_{0}, P2_{0}, P3_{0}, target_{0};
    uint32_t flags1_ = 0, flags2_ = 0;
    // race
    int node_ = 0, laps_ = 0, respawns_ = 0, wallHits_ = 0, lastRestart_ = 0;
    float lapTime_ = 0, lastLap_ = 0, bestLap_ = 0;
    bool started_ = false;
    int crossings_ = 0;
    float crossLapTime_ = 0;
    float lapSide_ = 0, checkTimer_ = 0;   // first-node plane side; neon_drone's 2 s progress check (+0x1714)
    std::array<glm::vec3, 3> checkHistory_{};   // its positions, oldest first (+0x1718 / +0x1724 / +0x1730)
    bool resetRequested_ = false;          // craft +5: respawn at the restart point this frame (0x5cb30)
    bool resetHeld_ = false;               // BACK held last frame (the game reacts to the press)
    float upsideDownTime_ = 0;             // +0x934
    int orthoCycle_ = 0;                // 0xeb9f0's rotating counter (0x5a216c)
    float latHist_[12]{};               // +0x173c.. lateral velocity history
    float sway_[3]{};                   // +0x17c8 close, +0x17cc mid, +0x17d0 far
    glm::vec3 camUp_{0, 1, 0};          // +0xcf4
    glm::vec3 upSmooth_{0, 1, 0};       // +0x17d4 (object_update: halfway to the craft's up every frame)
    float tiltHist_[64]{}, tilt_ = 0;   // +0xe8c.. head-tilt buffer, +0xe84 head tilt
    int tiltIdx_ = 0;
};

} // namespace argon
