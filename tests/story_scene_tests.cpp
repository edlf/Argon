// Argon: tests for the story scenes: which scene plays when (movie\story_scenes.hpp), the speech lines' keys
// (movie\speech_lines.hpp), posing with other tracks (Cmm::pose with BoneKeys), and with game_data the NMS dopesheet's
// timing (movie\nms_sequence.hpp) and the CMS animations (cms_animation.hpp); the head turns (bone_aim.hpp,
// movie\head_turn.hpp) and the scene's light (movie\scene_light.hpp). Run with test.bat / test.sh.
#include "model/bone_aim.hpp"
#include "check.hpp"
#include "model/cms_animation.hpp"
#include "movie/head_turn.hpp"
#include "movie/nms_sequence.hpp"
#include "movie/scene_light.hpp"
#include "movie/speech_lines.hpp"
#include "movie/story_scenes.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <glm/gtc/matrix_transform.hpp>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-3f;
}

// ---- which scene

void preRaceSceneNamesTheTrackAndPilot()
{
    CHECK(story::preRaceScene("silver", 0) == "silver_tara.nms");
}

void preRaceSceneUsesTheStoryNames()
{
    CHECK(story::preRaceScene("alt_bracken", 7) == "alt_bracken_kahekili.nms");
}

void amateurEndsWithTheNemesisBeaten()
{   // Tara's Amateur ends on Alicia's home route (track 8): Tara beats Alicia there
    CHECK(story::raceEndScene(1, 8, "alt_silver") == std::optional<std::string>("alt_silver_tara_beatnemesis.nms"));
}

void masterEndsWithTheNemesisBeaten()
{
    CHECK(story::raceEndScene(3, 0, "silver") == std::optional<std::string>("silver_alicia_beatnemesis.nms"));
}

void redshiftEndsWithTheWinningScene()
{
    CHECK(story::raceEndScene(4, 7, "draco") == std::optional<std::string>("draco_kahekili_winning.nms"));
}

void noviceHasNoEndScene()
{
    CHECK(!story::raceEndScene(0, 0, "silver"));
}

void marsHasNoEndScene()
{
    CHECK(!story::raceEndScene(4, 16, "mars"));
}

// ---- actors' pilots (0x9b170)

void longFolderMatchesItsPilot()
{
    CHECK(story::pilotOfFolder("kukailimoku") == 7);
}

void folderNamesMapToPilots()
{
    CHECK(story::pilotOfFolder("darius") == 3 && story::pilotOfFolder("abi") == 10);
}

void shipFolderIsNoPilot()
{
    CHECK(story::pilotOfFolder("feship_jordan") == -1);
}

// ---- speech lines (0x9aff0)

void speechKeyFindsItsLine()
{
    const std::optional<story::SpeechLine> line = story::speechLineFor("IDS_SILVER_YURI");
    CHECK(line && std::strcmp(line->wav, "tara\\tara_8.wav") == 0 && line->stringId == 1);
}

void emptySpeechKeyTakesTheFirstLine()
{   // silver_tara.nms's speech object names no key: the game plays IDS_SILVER_TARA
    const std::optional<story::SpeechLine> line = story::speechLineFor("");
    CHECK(line && std::strcmp(line->key, "IDS_SILVER_TARA") == 0);
}

void unknownSpeechKeyHasNoLine()
{
    CHECK(!story::speechLineFor("IDS_NOTHING"));
}

void speechLineNamesItsSpeaker()
{
    const std::optional<story::SpeechLine> line = story::speechLineFor("IDS_SILVER_NEMESIS_CHAT2");
    CHECK(line && line->speaker == 8 && std::strcmp(line->faceAnimation, "alicia\\alicia face 14b.cms") == 0);
}

void speechKeysCompareUpToTheShorter()
{
    CHECK(story::speechKeysMatch("IDS_TARA_WINNING", "IDS_TARA") && !story::speechKeysMatch("IDS_TARA_WINNING", "IDS_TARA_DEFEATED"));
}

// ---- posing with other tracks (0xb8630)

Cmm oneBoneModel()
{
    Cmm model;
    model.bones.resize(1);
    const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::memcpy(model.bones[0].local, identity, sizeof identity);
    return model;
}

// x = 0, 10, 20 at 10 keys a second
CmmTrack threePositionKeys()
{
    CmmTrack track;
    track.flags = 1;
    track.keys = 3;
    track.fps = 10.f;
    track.pos = {0, 0, 0, 10, 0, 0, 20, 0, 0};
    return track;
}

float poseX(const CmmTrack &track, float seconds, bool once)
{
    const Cmm model = oneBoneModel();
    std::vector<float> world;
    model.pose({BoneKeys{&track, seconds, once}}, world);
    return world[12];
}

void loopingTrackWrapsToItsFirstKey()
{   // frame 2.5: key 2 towards key 0
    CHECK(near(poseX(threePositionKeys(), 0.25f, false), 10.f));
}

void playOnceHoldsTheLastKey()
{
    CHECK(near(poseX(threePositionKeys(), 0.25f, true), 20.f));
}

void playOnceHoldsTheFirstKeyBeforeTheStart()
{   // the scenes start some animations at negative times (segment source start below 0)
    CHECK(near(poseX(threePositionKeys(), -0.5f, true), 0.f));
}

void playOnceBlendsBetweenKeys()
{
    CHECK(near(poseX(threePositionKeys(), 0.15f, true), 15.f));
}

void boneWithoutTrackKeepsItsLocal()
{
    const Cmm model = oneBoneModel();
    std::vector<float> world;
    model.pose(std::vector<BoneKeys>{}, world);
    CHECK(near(world[0], 1.f) && near(world[12], 0.f));
}

void tracksBindToBonesByName()
{
    Cmm model = oneBoneModel();
    model.bones[0].name = "tara_skel_head";
    CmsAnimation animation;
    animation.tracks.resize(2);
    animation.tracks[1].name = "tara_skel_head";
    const std::vector<const CmmTrack *> bound = tracksByBone(model, animation);
    CHECK(bound.size() == 1 && bound[0] == &animation.tracks[1]);
}

// ---- head turns (0xb8630 flags 0x2000 / 0x4000, 0x99360)

// a bone at (0, 0, 0) facing +z, its y up
const glm::mat4 kPosedBone(1.f);
const glm::vec3 kUnitRows(1.f);

bool nearVec(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-3f;
}

glm::mat4 aimedAt(const glm::vec3 &target, float share)
{
    return aimBone(kPosedBone, BoneAim{AimKind::TurnsChildren, target, share}, kUnitRows);
}

void fullShareAimsTheZAxisAtTheTarget()
{
    CHECK(nearVec(glm::vec3(aimedAt(glm::vec3(10, 0, 0), 1.f)[2]), glm::vec3(1, 0, 0)));
}

void fullShareKeepsTheYAxisUp()
{
    CHECK(nearVec(glm::vec3(aimedAt(glm::vec3(10, 0, 0), 1.f)[1]), glm::vec3(0, 1, 0)));
}

void turnedAxesStayRightHanded()
{
    const glm::mat4 aimed = aimedAt(glm::vec3(3, 4, 5), 0.6f);
    CHECK(nearVec(glm::cross(glm::vec3(aimed[0]), glm::vec3(aimed[1])), glm::vec3(aimed[2])));
}

void zeroShareKeepsThePose()
{
    CHECK(nearVec(glm::vec3(aimedAt(glm::vec3(10, 0, 0), 0.f)[2]), glm::vec3(0, 0, 1)));
}

void halfShareTurnsHalfway()
{   // z blended from (0, 0, 1) and (1, 0, 0) by a half: 45 degrees
    CHECK(nearVec(glm::vec3(aimedAt(glm::vec3(10, 0, 0), 0.5f)[2]), glm::normalize(glm::vec3(1, 0, 1))));
}

void turningKeepsTheTranslation()
{
    const glm::mat4 bone = glm::translate(glm::mat4(1.f), glm::vec3(1, 2, 3));
    const glm::mat4 aimed = aimBone(bone, BoneAim{AimKind::TurnsChildren, glm::vec3(10, 0, 0), 1.f}, kUnitRows);
    CHECK(nearVec(glm::vec3(aimed[3]), glm::vec3(1, 2, 3)));
}

void turnedRowsTakeTheBoneScale()
{
    const glm::mat4 aimed = aimBone(kPosedBone, BoneAim{AimKind::TurnsChildren, glm::vec3(10, 0, 0), 1.f}, glm::vec3(2, 3, 4));
    CHECK(near(glm::length(glm::vec3(aimed[0])), 2.f) && near(glm::length(glm::vec3(aimed[1])), 3.f) && near(glm::length(glm::vec3(aimed[2])), 4.f));
}

void targetOnTheBoneLeavesItAsPosed()
{
    CHECK(nearVec(glm::vec3(aimedAt(glm::vec3(0.f), 1.f)[2]), glm::vec3(0, 0, 1)));
}

Cmm namedBones(const std::vector<std::string> &names)
{
    Cmm model;
    model.bones.resize(names.size());
    for (size_t b = 0; b < names.size(); ++b)
    {
        model.bones[b].name = names[b];
    }
    return model;
}

void headTurnsByTheSegmentsShare()
{
    const std::vector<BoneAim> aims = story::headTurnAims(namedBones({"tara_skel_head"}), glm::vec3(1.f), 0.6f);
    CHECK(aims[0].kind == AimKind::TurnsChildren && near(aims[0].share, 0.6f));
}

void neckTurnsAThirdOfTheShareAlone()
{
    const std::vector<BoneAim> aims = story::headTurnAims(namedBones({"tara_skel_neck"}), glm::vec3(1.f), 0.6f);
    CHECK(aims[0].kind == AimKind::KeepsChildren && near(aims[0].share, 0.18f));
}

void eyesLookStraightAtTheTarget()
{
    const std::vector<BoneAim> aims = story::headTurnAims(namedBones({"eyeL", "eyeR"}), glm::vec3(1.f), 0.6f);
    CHECK(near(aims[0].share, 1.f) && near(aims[1].share, 1.f) && aims[1].kind == AimKind::TurnsChildren);
}

void eyeNamesMustMatchExactly()
{   // 0x99b0d compares five bytes, the NUL included
    const std::vector<BoneAim> aims = story::headTurnAims(namedBones({"tara_eyeL_up_RIG_", "tara_skel_hips"}), glm::vec3(1.f), 0.6f);
    CHECK(aims[0].kind == AimKind::None && aims[1].kind == AimKind::None);
}

void lookedAtBoneIsTheLastHead()
{
    CHECK(story::lookedAtBone(namedBones({"a_skel_head", "b", "c_skel_head_2"})) == 2);
}

void modelWithoutHeadIsLookedAtByItsOrigin()
{
    CHECK(story::lookedAtBone(namedBones({"hull", "fin"})) == -1);
}

void objectZeroIsNeverLookedAt()
{   // 0x998b2: jle
    CHECK(!story::turnsHead(0, 30) && !story::turnsHead(-1, 30) && !story::turnsHead(30, 30) && story::turnsHead(29, 30));
}

// a neck at (0, 1, 0) with a head one unit above it
Cmm neckAndHead()
{
    Cmm model = namedBones({"skel_neck", "skel_head"});
    const float raised[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1};
    std::memcpy(model.bones[0].local, raised, sizeof raised);
    std::memcpy(model.bones[1].local, raised, sizeof raised);
    model.bones[1].parent = 0;
    return model;
}

std::vector<float> poseNeckTurnedTo(AimKind kind)
{
    std::vector<float> world;
    neckAndHead().pose({}, {BoneAim{kind, glm::vec3(10, 1, 0), 1.f}, BoneAim{}}, world);
    return world;
}

void turnedNeckFacesTheTarget()
{   // its z row (8..10) along +x
    CHECK(near(poseNeckTurnedTo(AimKind::KeepsChildren)[8], 1.f));
}

void neckKeepsItsChildAsPosed()
{   // 0x4000: the children hang off the unturned neck (+0xc0); the head's z row stays +z
    const std::vector<float> world = poseNeckTurnedTo(AimKind::KeepsChildren);
    CHECK(near(world[16 + 8], 0.f) && near(world[16 + 10], 1.f));
}

void turnedHeadCarriesItsChildren()
{   // 0x2000: the children hang off the turned bone
    CHECK(near(poseNeckTurnedTo(AimKind::TurnsChildren)[16 + 8], 1.f));
}

// ---- the scene's light (0xb4f80)

void sceneLightShinesAlongTheStoredDirection()
{
    const story::SceneLight light = story::sceneLight(glm::vec3(1, -1, 1), 0xff808080u);
    CHECK(nearVec(light.direction, glm::normalize(glm::vec3(1, -1, 1))) && nearVec(light.colour, glm::vec3(128.f / 255.f)));
}

void zeroDirectionGivesNoLight()
{
    const story::SceneLight light = story::sceneLight(glm::vec3(0.f), 0xff808080u);
    CHECK(nearVec(light.direction, glm::vec3(0, 1, 0)) && nearVec(light.colour, glm::vec3(0.f)));
}

void directionAboveAThousandGivesNoLight()
{
    CHECK(nearVec(story::sceneLight(glm::vec3(1001, 0, 0), 0xffffffffu).colour, glm::vec3(0.f)));
}

void largeNegativeDirectionIsKept()
{   // only the upper bound is checked
    CHECK(nearVec(story::sceneLight(glm::vec3(-2000, 0, 0), 0xffffffffu).colour, glm::vec3(1.f)));
}

void brokenDirectionGivesNoLight()
{
    CHECK(nearVec(story::sceneLight(glm::vec3(std::nanf(""), 1, 0), 0xffffffffu).colour, glm::vec3(0.f)));
}

// ---- with game_data

std::filesystem::path gameData()
{
    for (std::filesystem::path dir = QR_SOURCE_DIR; dir.has_relative_path(); dir = dir.parent_path())
    {
        if (std::filesystem::exists(dir / "game_data" / "data" / "movies"))
        {
            return dir / "game_data" / "data";
        }
    }
    return {};
}

std::vector<uint8_t> readBytes(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

NmsSequence beatNemesisScene(const std::filesystem::path &data)
{
    NmsSequence scene;
    scene.load(readBytes(data / "movies" / "silver" / "silver_alicia_beatnemesis.nms"));
    return scene;
}

const SequenceActor *actorOf(const SequenceFrame &frame, int object)
{
    for (const SequenceActor &actor : frame.actors)
    {
        if (actor.object == object)
        {
            return &actor;
        }
    }
    return nullptr;
}

void sceneLengthIsTheCameraTrack(const std::filesystem::path &data)
{
    CHECK(near(beatNemesisScene(data).length(), 18.4f));
}

void speechPlaysFromItsSegmentStart(const std::filesystem::path &data)
{   // "Alicia's beaten Tara speech" (object 18) from 3.3 s
    const SequenceFrame frame = beatNemesisScene(data).frameAt(4.0f);
    CHECK(frame.speech.object == 18 && near(frame.speech.seconds, 0.7f));
}

void groupSegmentStopsTheSpeech(const std::filesystem::path &data)
{   // 11.3 .. 12.4: "All Actors"
    CHECK(beatNemesisScene(data).frameAt(12.f).speech.object == -1);
}

void actorAnimationTimeFollowsItsSegment(const std::filesystem::path &data)
{   // Tara's "tara 05_2.cms" (object 13) from source -10.7: at 4 s it is at -6.7 (key 0 held)
    NmsSequence scene = beatNemesisScene(data);
    const SequenceActor *tara = actorOf(scene.frameAt(4.f), 9);
    CHECK(tara && tara->animation == 13 && near(tara->seconds, -6.7f) && !tara->loops);
}

void secondActorHasItsOwnAnimation(const std::filesystem::path &data)
{
    NmsSequence scene = beatNemesisScene(data);
    const SequenceActor *alicia = actorOf(scene.frameAt(4.f), 11);
    CHECK(alicia && alicia->animation == 14 && near(alicia->seconds, 1.6f));
}

void cameraCutPicksTheSegmentsCamera(const std::filesystem::path &data)
{   // 0 .. 3 s: "Camera 2" (object 15)
    const std::optional<SequenceCamera> camera = beatNemesisScene(data).cameraAt(1.f);
    CHECK(camera && camera->object == 15);
}

void embeddedAnimationsAreRead(const std::filesystem::path &data)
{
    const NmsSequence scene = beatNemesisScene(data);
    const CmsAnimation *animation = scene.animation(13);
    CHECK(animation && near(animation->seconds, 7.7f) && animation->tracks.size() == 121);
}

void faceAnimationFileReads(const std::filesystem::path &data)
{
    CmsAnimation face;
    const std::vector<uint8_t> bytes = readBytes(data / "characters" / "tara" / "tara face 01.cms");
    const std::optional<size_t> end = readCms(bytes, 0, face);
    CHECK(end && *end == bytes.size() && near(face.seconds, 4.4f) && face.tracks.size() == 74);
}

void gloryFlybyStillLoads(const std::filesystem::path &data)
{
    NmsSequence glory;
    CHECK(glory.load(readBytes(data / "movies" / "silver" / "silver_glory.nms")) && glory.cameraAt(1.f).has_value());
}

NmsSequence taraAndJordanScene(const std::filesystem::path &data)
{
    NmsSequence scene;
    scene.load(readBytes(data / "movies" / "silver" / "silver_jordan.nms"));
    return scene;
}

void taraTurnsToJordan(const std::filesystem::path &data)
{   // Tara (object 9) 0 .. 8 s: flag 2 towards Jordan (object 13), 0.59
    NmsSequence scene = taraAndJordanScene(data);
    const SequenceActor *tara = actorOf(scene.frameAt(2.f), 9);
    CHECK(tara && tara->lookAt == 13 && tara->looksAtActor && near(tara->lookShare, 0.59f));
}

void jordanTurnsToTara(const std::filesystem::path &data)
{
    NmsSequence scene = taraAndJordanScene(data);
    const SequenceActor *jordan = actorOf(scene.frameAt(2.f), 13);
    CHECK(jordan && jordan->lookAt == 9 && near(jordan->lookShare, 0.6f));
}

void sceneLightComesFromTheHeader(const std::filesystem::path &data)
{   // (-0.8, -1, -1.19), 0xff808080
    const story::SceneLight light = taraAndJordanScene(data).light();
    CHECK(nearVec(light.direction, glm::normalize(glm::vec3(-0.8f, -1.f, -1.19f))) && near(light.colour.r, 128.f / 255.f));
}

void everySceneOnTheDiscLoads(const std::filesystem::path &data)
{
    int scenes = 0, loaded = 0;
    for (const auto &folder : std::filesystem::directory_iterator(data / "movies"))
    {
        if (!folder.is_directory())
        {
            continue;
        }
        for (const auto &file : std::filesystem::directory_iterator(folder.path()))
        {
            if (file.path().extension() != ".nms")
            {
                continue;
            }
            NmsSequence scene;
            ++scenes;
            loaded += scene.load(readBytes(file.path())) ? 1 : 0;
        }
    }
    CHECK(scenes > 200 && loaded == scenes);
}

void runGameDataTests()
{
    const std::filesystem::path data = gameData();
    if (data.empty())
    {
        std::printf("game_data\\data\\movies not found: the file tests are skipped\n");
        return;
    }
    sceneLengthIsTheCameraTrack(data);
    speechPlaysFromItsSegmentStart(data);
    groupSegmentStopsTheSpeech(data);
    actorAnimationTimeFollowsItsSegment(data);
    secondActorHasItsOwnAnimation(data);
    cameraCutPicksTheSegmentsCamera(data);
    embeddedAnimationsAreRead(data);
    faceAnimationFileReads(data);
    gloryFlybyStillLoads(data);
    taraTurnsToJordan(data);
    jordanTurnsToTara(data);
    sceneLightComesFromTheHeader(data);
    everySceneOnTheDiscLoads(data);
}
} // namespace

int main()
{
    preRaceSceneNamesTheTrackAndPilot();
    preRaceSceneUsesTheStoryNames();
    amateurEndsWithTheNemesisBeaten();
    masterEndsWithTheNemesisBeaten();
    redshiftEndsWithTheWinningScene();
    noviceHasNoEndScene();
    marsHasNoEndScene();
    longFolderMatchesItsPilot();
    folderNamesMapToPilots();
    shipFolderIsNoPilot();
    speechKeyFindsItsLine();
    emptySpeechKeyTakesTheFirstLine();
    unknownSpeechKeyHasNoLine();
    speechLineNamesItsSpeaker();
    speechKeysCompareUpToTheShorter();
    loopingTrackWrapsToItsFirstKey();
    playOnceHoldsTheLastKey();
    playOnceHoldsTheFirstKeyBeforeTheStart();
    playOnceBlendsBetweenKeys();
    boneWithoutTrackKeepsItsLocal();
    tracksBindToBonesByName();
    fullShareAimsTheZAxisAtTheTarget();
    fullShareKeepsTheYAxisUp();
    turnedAxesStayRightHanded();
    zeroShareKeepsThePose();
    halfShareTurnsHalfway();
    turningKeepsTheTranslation();
    turnedRowsTakeTheBoneScale();
    targetOnTheBoneLeavesItAsPosed();
    headTurnsByTheSegmentsShare();
    neckTurnsAThirdOfTheShareAlone();
    eyesLookStraightAtTheTarget();
    eyeNamesMustMatchExactly();
    lookedAtBoneIsTheLastHead();
    modelWithoutHeadIsLookedAtByItsOrigin();
    objectZeroIsNeverLookedAt();
    turnedNeckFacesTheTarget();
    neckKeepsItsChildAsPosed();
    turnedHeadCarriesItsChildren();
    sceneLightShinesAlongTheStoredDirection();
    zeroDirectionGivesNoLight();
    directionAboveAThousandGivesNoLight();
    largeNegativeDirectionIsKept();
    brokenDirectionGivesNoLight();
    runGameDataTests();
    std::printf("story scene tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
