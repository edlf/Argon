// Argon: the props' models (prop_models.hpp).
#include "render/prop_models.hpp"

#include "files/game_path.hpp"

namespace
{

Found<Track> built(Track &model)
{
    return model.vb ? Found<Track>(model) : Found<Track>();
}

template <typename Key> Found<Track> builtIn(std::map<Key, Track> &models, const Key &key)
{
    auto known = models.find(key);
    return known != models.end() ? built(known->second) : Found<Track>();
}

template <typename Key> std::vector<std::string> foldersOf(const std::map<Key, Track> &models)
{
    std::vector<std::string> folders;
    for (const auto &[folder, model] : models)
    {
        folders.push_back(folder);
    }
    return folders;
}

}  // namespace

void PropModels::create(ModelBuilder &builder, argon::gfx::SetPool pool, const std::string &gameFolder)
{
    builder_ = &builder;
    pool_ = pool;
    game_ = gameFolder;
}

Found<Track> PropModels::craft(const std::string &pilot)
{
    if (craft_.count(pilot))
    {
        return builtIn(craft_, pilot);
    }
    ModelSource source = readModel(game_ + "/data/craft/" + pilot);
    adoptCraft(pilot, source);
    return built(craft_[pilot]);
}

Found<Track> PropModels::loadedCraft(const std::string &pilot)
{
    return builtIn(craft_, pilot);
}

void PropModels::adoptCraft(const std::string &pilot, ModelSource &source)
{
    if (craft_.count(pilot))
    {
        return;
    }
    builder_->build(craftSlot(pilot), source, craftUse());
}

Found<Track> PropModels::pickup(const std::string &folder)
{
    if (pickups_.count(folder))
    {
        return builtIn(pickups_, folder);
    }
    Track &model = pickupSlot(folder);
    const std::string path = game_ + "/data/objects/" + folder;
    if (argon::gamePathExists(path + "/model.cmm"))
    {
        builder_->load(model, path, pickupUse());
    }
    return built(model);
}

Found<Track> PropModels::explosion(int model)
{
    if (explosions_.count(model))
    {
        return builtIn(explosions_, model);
    }
    Track &explosionModel = explosions_[model];
    explosionModel.name = "explosion_type" + std::to_string(model);
    const std::string path = game_ + "/data/objects/" + explosionModel.name;
    if (argon::gamePathExists(path + "/model.cmm"))
    {
        builder_->load(explosionModel, path, ModelUse{.pool = pool_});
    }
    return built(explosionModel);
}

Found<Track> PropModels::loadedExplosion(int model)
{
    return builtIn(explosions_, model);
}

Track &PropModels::craftSlot(const std::string &pilot)
{
    Track &model = craft_[pilot];
    model.name = pilot;
    return model;
}

Track &PropModels::pickupSlot(const std::string &folder)
{
    Track &model = pickups_[folder];
    model.name = folder;
    return model;
}

// every race craft: a prop whose shadow hull only casts the ship shadow
ModelUse PropModels::craftUse() const
{
    return ModelUse{false, pool_, true, true};
}

ModelUse PropModels::pickupUse() const
{
    return ModelUse{.pool = pool_, .prop = true, .reskinned = true};
}

std::vector<std::string> PropModels::craftFolders() const
{
    return foldersOf(craft_);
}

std::vector<std::string> PropModels::pickupFolders() const
{
    return foldersOf(pickups_);
}

void PropModels::releaseAll()
{
    for (auto &[pilot, model] : craft_)
    {
        builder_->release(model);
    }
    for (auto &[folder, model] : pickups_)
    {
        builder_->release(model);
    }
    for (auto &[type, model] : explosions_)
    {
        builder_->release(model);
    }
}
