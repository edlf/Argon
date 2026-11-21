// Argon: the editor's markings, see editor_markings.hpp.
#include "render/editor_markings.hpp"

#include "render/scene_recorder.hpp"
#include "render/scene_toggles.hpp"

namespace
{
void drawLineRange(argon::gfx::CommandList &commands, bool shown, const uint32_t range[2])
{
    if (shown && range[1])
    {
        commands.draw({range[1], range[0]});
    }
}
}  // namespace

// the editor's markings (start grid, AI route, power-ups, sound emitters, route issues)
void recordEditorMarkings(const EditorMarkingSources &sources, const SceneRecorder &scene)
{
    argon::gfx::CommandList &commands = scene.commands();
    const Track &track = sources.track;
    const SceneToggles &shown = sources.toggles;
    const bool anyShown = shown.starts || shown.aiRoute || shown.pickups || shown.emitters || shown.issues;
    if (!track.lines || !anyShown)
    {
        return;
    }
    commands.bindPipeline(sources.linePipe);
    scene.setDepthCull({.test = !shown.markersOnTop, .write = !shown.markersOnTop, .cullBack = false});
    scene.bindVertexBuffer(track.lines);
    scene.pushMatrices(scene.viewProj(), glm::mat4(1));
    drawLineRange(commands, shown.starts, track.lineStarts);
    drawLineRange(commands, shown.aiRoute, track.lineAi);
    drawLineRange(commands, shown.pickups, track.linePups);
    drawLineRange(commands, shown.emitters, track.lineSounds);
    drawLineRange(commands, shown.issues, track.lineIssues);
}
