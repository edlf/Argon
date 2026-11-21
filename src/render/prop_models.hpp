// Argon: the props' models, loaded on demand and kept across tracks: the race craft (data\craft\<pilot>), the
// pickups (data\objects\<folder>) and the explosions (data\objects\explosion_type<n>).
#pragma once
#include "util/found.hpp"
#include "model/model_source.hpp"
#include "render/gpu_model.hpp"
#include "render/model_builder.hpp"

#include <map>
#include <string>
#include <vector>

class PropModels
{
  public:
    // pool: the props' descriptor pool, kept for the session
    void create(ModelBuilder &builder, argon::gfx::SetPool pool, const std::string &gameFolder);

    // the pilot's ship, read and built now if it is not loaded yet; a failed build stays (no vertex buffer), so the
    // folder is not read again
    Found<Track> craft(const std::string &pilot);
    Found<Track> loadedCraft(const std::string &pilot);
    // a race craft read on a worker (ui\fe_models.cpp's preload); one loaded first is kept
    void adoptCraft(const std::string &pilot, ModelSource &source);
    Found<Track> pickup(const std::string &folder);
    // loaded when its first explosion starts (0x7dd40 loads all 16 at start-up)
    Found<Track> explosion(int model);
    Found<Track> loadedExplosion(int model);

    // a track load's reads (track\track_load.hpp): the model each fills, and how it is built
    Track &craftSlot(const std::string &pilot);
    Track &pickupSlot(const std::string &folder);
    ModelUse craftUse() const;
    ModelUse pickupUse() const;

    std::vector<std::string> craftFolders() const;
    std::vector<std::string> pickupFolders() const;
    void releaseAll();

  private:
    ModelBuilder *builder_ = nullptr;
    argon::gfx::SetPool pool_;
    std::string game_;
    std::map<std::string, Track> craft_, pickups_;
    std::map<int, Track> explosions_;
};
