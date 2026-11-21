// Argon: the front end's models, the shell's (fe_shell_ctor 0x2f4b0, loaded by main_menu 0x30af0: +0x4458
// titlelogo, +0x3e48 dodechstart, +0x1ff8.. the main menu items, +0x4a68 earth, +0x5078 mars) and the characters' and
// ships' (frontend\characters / shipmodel, the game loads the picked one in the background; the viewer keeps all 16 of
// each for the session). All of them and the race craft are read on a worker thread from the start (ModelReadQueue)
// and uploaded one a frame; a model a page needs before its turn loads at once (ui\fe_models.cpp).
#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "logic/start_grid.hpp"
#include "render/model_builder.hpp"
#include "render/prop_models.hpp"
#include "render/upload_batches.hpp"
#include "gfx/gfx_types.hpp"
#include "model/model_read_queue.hpp"
#include "ui/fe_scene.hpp"

// a character model is cut at the characters' alpha reference (kCharacterAlphaRef); a trophy has no weapon parts; the
// others keep the load's
enum class FeModelKind
{
    Plain,
    Character,
    Trophy
};

class FeModels
{
  public:
    static constexpr int kMenuItems = 5;   // FE MAIN's rows with a model: Tournament .. Options

    // what the models load with: the game's files, the builder and the props' pool, the pilots
    struct Sources
    {
        const std::string &gameFolder;
        ModelBuilder &models;
        UploadBatches &uploads;   // the models read at once go up in one submission
        const argon::gfx::SetPool &propPool;
        PropModels &props;   // the craft folders read for the menus become the race's props
        const StartGrid &startGrid;
    };
    explicit FeModels(const Sources &sources);
    ~FeModels();
    FeModels(const FeModels &) = delete;
    FeModels &operator=(const FeModels &) = delete;

    void loadTitle();      // frontend\titlelogo, frontend\dodechstart
    void loadGlobes();      // frontend\earth, frontend\mars
    void uploadPreloaded();       // the models the worker has read, a few ms a frame (starts the worker the first time)
    bool preloaded() const;       // every model uploaded: the front end may be skipped through
    void unloadAll();             // App::shutdown, before the device goes

    Track &menuItem(int row);   // frontend\mainmenu\<row>
    Track &titleLogo()
    {
        return logo;
    }
    Track &dodecahedron()
    {
        return dodech;
    }
    Track &earth()
    {
        return earthModel;
    }
    Track &mars()
    {
        return marsModel;
    }
    Track &character(int pilot)
    {
        return load(characters, "characters", pilot);
    }
    Track &ship(int pilot)
    {
        return load(ships, "shipmodel", pilot);
    }
    Track &trophy(int level);   // objects\fe_trophy1..5 (FE AWARDS' enter 0x12e70), loaded on first use
    Track &speedometer();       // frontend\speedselect (shell +0x5aa0, main_menu 0x30ea9), loaded on first use
    // FE SPEEDSELECT / FE TOURNEYTYPE (0x33fa0 / 0x35e50): the speedometer where its model puts it, seen by camera 1,
    // its SPEEDO_NEEDLE turned to the speed / level (0xb99f0)
    FeScene speedometerScene(int level);
    static glm::vec3 characterPosition(int pilot);   // 0x1b9a88: where camera 2 sees the pilot's character

    // a page shows the pilot's character / ship: a different one restarts its clock (the model fade and the
    // character's animation, 0x15db0; FE SHIP INFO +0x1e0)
    void showCharacter(int pilot);
    void showShip(int pilot);
    bool characterLoaded(int pilot) const
    {
        return bool(characters[size_t(pilot & 15)].vb);
    }
    bool characterShowing(int pilot) const
    {
        return shownCharacter == pilot && characters[size_t(pilot & 15)].vb;
    }
    double characterShownAt() const
    {
        return characterSince;
    }
    double shipShownAt() const
    {
        return shipSince;
    }

    void freezePoses()
    {
        animated = false;
    }   // --no-anim: the characters stay in their stored pose (comparison)
    bool posesAnimated() const
    {
        return animated;
    }

  private:
    // one queued folder: a front-end model (its Track and name) or, without a Track, the race craft of that pilot
    struct Preload
    {
        Track *model = nullptr;
        std::string name, folder;
        bool character = false;
    };

    Track &load(std::array<Track, 16> &models, const char *kind, int pilot);
    void loadNow(Track &model, const std::string &name, const std::string &folder, FeModelKind kind);
    void startPreload();
    std::vector<Preload> preloadList();
    void adopt(const Preload &preload, ModelSource &source);

    Sources sources_;
    Track items[kMenuItems];
    Track logo, dodech, earthModel, marsModel, speedo;   // a Track with a name has been loaded (or tried)
    std::array<Track, 16> characters, ships;
    std::array<Track, 5> trophies;
    std::vector<Preload> preloads;
    std::unique_ptr<ModelReadQueue> reads;
    int shownCharacter = -1, shownShip = -1;
    double characterSince = 0, shipSince = 0;
    bool animated = true;
};
