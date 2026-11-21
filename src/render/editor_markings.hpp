// Argon: the editor's markings over the scene (start grid, AI route, power-ups, sound emitters, route issues), a
// viewer extra; nothing outside the editor.
#pragma once
#include "gfx/gfx_types.hpp"

class SceneRecorder;
struct SceneToggles;
struct Track;

// what the markings draw from: the track's marker lines, the markings shown, the line pipeline
struct EditorMarkingSources
{
    const Track &track;
    const SceneToggles &toggles;
    const argon::gfx::Pipeline &linePipe;
};

void recordEditorMarkings(const EditorMarkingSources &sources, const SceneRecorder &scene);
