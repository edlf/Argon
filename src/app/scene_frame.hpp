// Argon: the frame as the 3D scene records it (render\scene_sources.hpp SceneHost): the frame slot in flight and the
// view being recorded (with the per-view track sector meshes), the windings, the clocks, and what the scene draws on
// the race's behalf: the spray texture the first time, the pickup models posed, the countdown digits and the
// editor's markings. What the scene asks about the race itself is App's (SceneRaceView).
#pragma once
#include <array>
#include <string>

#include "app/track_arrival.hpp"
#include "render/scene_sources.hpp"

class CountdownDigits;
class EditorSession;
class RaceEggs;
class RaceStart;
struct SceneToggles;

// the frame slot being prepared and recorded (presenter().frameSlot()) and the view in it being recorded
struct FrameSlots
{
    int frame = 0;
    int view = 0;
};

struct SceneFrameSources
{
    Renderer &renderer;
    ModelBuilder &models;                                       // the pickup models' poses
    const Track &track;
    const SceneToggles &toggles;                                // the winding (--cw / --ccw), the markings shown
    const RaceEggs &raceEggs;                                   // Mirror Tracks flips the scene's winding
    const std::string &game;                                    // the spray texture's folder
    const PickupModels &pickupModels, &takenPickupModels;
    const RaceStart &raceStart;                                 // the countdown's clock
    CountdownDigits &countdownDigits;                           // drawn over the 3D view
    const EditorSession &editor;                                // its markings, in the editor only
};

class SceneFrame : public SceneHost
{
  public:
    static constexpr int kMaxViews = 4;   // split screen: up to four views a frame (0x7efd0)

    explicit SceneFrame(const SceneFrameSources &sources) : sources_(sources)
    {
    }

    FrameSlots &slots()
    {
        return slots_;
    }
    std::array<TrackSectorMeshes, kMaxViews> &sectorMeshes()   // per view (render\scene_sources.hpp)
    {
        return sectorMeshes_;
    }
    ModelClock modelClock(float seconds) const override;   // an animated model's pose at that clock, into this frame's buffer

    int viewSlot() const override;
    argon::gfx::Winding sceneFrontFace() const override;   // the 3D view's: flipped with Mirror Tracks
    argon::gfx::Winding modelFrontFace() const override;   // the models' own winding (--cw / --ccw)
    double secondsSinceStart() const override;
    void loadSprayTexture() override;
    void animatePickupModels(float seconds) override;      // 0xb83e0: the pickup models' tracks on the clock
    void drawCountdown(SceneRecorder &recorder) override;
    void drawEditorMarkings(const SceneRecorder &recorder) const override;

  private:
    SceneFrameSources sources_;
    FrameSlots slots_;
    std::array<TrackSectorMeshes, kMaxViews> sectorMeshes_;
};
