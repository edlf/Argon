// Argon: the front end's models (ui\fe_models.hpp)
#include "ui/fe_models.hpp"

#include "craft/craftsim.hpp"
#include <GLFW/glfw3.h>
#include "logic/tourney_rules.hpp"

namespace
{
const char *const kMenuItemFolders[FeModels::kMenuItems] = {"story", "timeattack", "quickrace", "multiplayer", "options"};
// the characters' translucent alpha-test reference (xemu: hard hair edges); the ships keep the game's 0, their
// canopies are vertex alpha 123 (0.48) and would be cut away
constexpr float kCharacterAlphaRef = 0.5f;
constexpr double kUploadBudgetSeconds = 0.004;   // a frame's share for uploading preloaded models
// 0x1b9a88 (rotation 0: 0x5a2160..68 are never written); FE CHARSELECT / CHAR INFO and FE AWARDS use it
const float kCharacterPositions[16][3] = {{-6, -24, -77},
                                          {-4, -33, -80},
                                          {-6, -21, -80},
                                          {-5, -29, -88},
                                          {-5, -23, -80},
                                          {-6, -29, -80},
                                          {-4, -28, -80},
                                          {-7, -29, -80},
                                          {-4, -27, -80},
                                          {-4, -35, -80},
                                          {-3, -21, -80},
                                          {-5, -32, -87},
                                          {-9, -28, -80},
                                          {-1, -25, -80},
                                          {-3, -29, -80},
                                          {-2, -46, -103}};
}  // namespace

glm::vec3 FeModels::characterPosition(int pilot)
{
    const float *position = kCharacterPositions[pilot & 15];
    return glm::vec3(position[0], position[1], position[2]);
}

FeModels::FeModels(const Sources &sources) : sources_(sources)
{
}

FeModels::~FeModels() = default;

Track &FeModels::menuItem(int row)
{
    Track &model = items[size_t(row)];
    loadNow(model, kMenuItemFolders[row], sources_.gameFolder + "/data/frontend/mainmenu/" + kMenuItemFolders[row], FeModelKind::Plain);
    return model;
}

void FeModels::loadTitle()
{
    loadNow(logo, "titlelogo", sources_.gameFolder + "/data/frontend/titlelogo", FeModelKind::Plain);
    loadNow(dodech, "dodechstart", sources_.gameFolder + "/data/frontend/dodechstart", FeModelKind::Plain);
}

void FeModels::loadGlobes()
{
    loadNow(earthModel, "earth", sources_.gameFolder + "/data/frontend/earth", FeModelKind::Plain);
    loadNow(marsModel, "mars", sources_.gameFolder + "/data/frontend/mars", FeModelKind::Plain);
}

// the pilot's model of frontend\<kind>
Track &FeModels::load(std::array<Track, 16> &models, const char *kind, int pilot)
{
    Track &model = models[size_t(pilot & 15)];
    const std::string name = argon::pilotFolder(pilot & 15);
    loadNow(model, name, sources_.gameFolder + "/data/frontend/" + kind + "/" + name, &models == &characters ? FeModelKind::Character : FeModelKind::Plain);
    return model;
}

Track &FeModels::trophy(int level)
{
    const int trophy = std::clamp(level, 0, 4) + 1;
    const std::string name = "fe_trophy" + std::to_string(trophy);
    Track &model = trophies[size_t(trophy - 1)];
    loadNow(model, name, sources_.gameFolder + "/data/objects/" + name, FeModelKind::Trophy);
    return model;
}

Track &FeModels::speedometer()
{
    loadNow(speedo, "speedselect", sources_.gameFolder + "/data/frontend/speedselect", FeModelKind::Plain);
    return speedo;
}

FeScene FeModels::speedometerScene(int level)
{
    FeScene scene;
    scene.models[0] = &speedometer();
    scene.turnedBone = "SPEEDO_NEEDLE";
    const float needle = glm::radians(argon::tourney::speedoNeedleDegrees(level));
    scene.boneTurn = glm::rotate(glm::mat4(1.f), needle, glm::vec3(0.f, 0.f, 1.f));
    return scene;
}

// read and upload on this frame, unless loaded already (by the preload or an earlier call)
void FeModels::loadNow(Track &model, const std::string &name, const std::string &folder, FeModelKind kind)
{
    if (!model.name.empty())
    {
        return;
    }
    model.name = name;
    if (kind == FeModelKind::Character)
    {
        model.translucentAlphaRef = kCharacterAlphaRef;
    }
    sources_.models.load(model, folder, ModelUse{.pool = sources_.propPool, .prop = true, .reskinned = true, .weaponParts = kind != FeModelKind::Trophy});
}

// the order the front end needs them: the title, the main menu, the globes, the characters, the ships, the race craft
std::vector<FeModels::Preload> FeModels::preloadList()
{
    const std::string frontend = sources_.gameFolder + "/data/frontend/";
    std::vector<Preload> list = {{&logo, "titlelogo", frontend + "titlelogo"}, {&dodech, "dodechstart", frontend + "dodechstart"}};
    for (int i = 0; i < kMenuItems; ++i)
    {
        list.push_back({&items[i], kMenuItemFolders[i], frontend + "mainmenu/" + kMenuItemFolders[i]});
    }
    list.push_back({&earthModel, "earth", frontend + "earth"});
    list.push_back({&marsModel, "mars", frontend + "mars"});
    for (int pilot = 0; pilot < 16; ++pilot)
    {
        const std::string name = argon::pilotFolder(pilot);
        list.push_back({&characters[size_t(pilot)], name, frontend + "characters/" + name, true});
    }
    for (int pilot = 0; pilot < 16; ++pilot)
    {
        const std::string name = argon::pilotFolder(pilot);
        list.push_back({&ships[size_t(pilot)], name, frontend + "shipmodel/" + name});
    }
    for (const std::string &pilot : sources_.startGrid.pilots)
    {
        list.push_back({nullptr, pilot, sources_.gameFolder + "/data/craft/" + pilot});
    }
    return list;
}

void FeModels::startPreload()
{
    preloads = preloadList();
    std::vector<std::string> folders;
    for (const Preload &preload : preloads)
    {
        folders.push_back(preload.folder);
    }
    reads = std::make_unique<ModelReadQueue>(std::move(folders));
}

// the models read so far, while this frame's budget lasts; their uploads go to the GPU together, without waiting for
// it (GpuContext::beginUploadBatch: alone, each texture waited for the frame being drawn)
void FeModels::uploadPreloaded()
{
    if (!reads)
    {
        startPreload();
    }
    const double until = glfwGetTime() + kUploadBudgetSeconds;
    sources_.uploads.beginUploadBatch();
    size_t index = 0;
    ModelSource source;
    while (glfwGetTime() < until && reads->takeNext(index, source))
    {
        adopt(preloads[index], source);
    }
    sources_.uploads.submitUploadBatch();
}

bool FeModels::preloaded() const
{
    return reads && reads->allTaken();
}

// a model already loaded on demand keeps its first copy
void FeModels::adopt(const Preload &preload, ModelSource &source)
{
    if (!preload.model)
    {
        sources_.props.adoptCraft(preload.name, source);
        return;
    }
    Track &model = *preload.model;
    if (!model.name.empty())
    {
        return;
    }
    model.name = preload.name;
    if (preload.character)
    {
        model.translucentAlphaRef = kCharacterAlphaRef;
    }
    sources_.models.build(model, source, ModelUse{.pool = sources_.propPool, .prop = true, .reskinned = true});
}

void FeModels::showCharacter(int pilot)
{
    character(pilot);
    if (shownCharacter == pilot)
    {
        return;
    }
    shownCharacter = pilot;
    characterSince = glfwGetTime();
}

void FeModels::showShip(int pilot)
{
    ship(pilot);
    if (shownShip == pilot)
    {
        return;
    }
    shownShip = pilot;
    shipSince = glfwGetTime();
}

void FeModels::unloadAll()
{
    reads.reset();   // the worker stops before the models go
    for (auto &character : characters)
    {
        sources_.models.release(character);
    }
    for (auto &ship : ships)
    {
        sources_.models.release(ship);
    }
    for (auto &item : items)
    {
        sources_.models.release(item);
    }
    for (auto &trophy : trophies)
    {
        sources_.models.release(trophy);
    }
    sources_.models.release(logo);
    sources_.models.release(dodech);
    sources_.models.release(earthModel);
    sources_.models.release(marsModel);
    sources_.models.release(speedo);
}
