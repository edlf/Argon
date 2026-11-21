// Argon: the frame copies the screens draw back (render\frame_copies.hpp).
#include "render/frame_copies.hpp"

#include "render/renderer.hpp"
#include "render/projection.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace
{
constexpr double kTransitionSeconds = 0.25;
}  // namespace

FrameCopies::FrameCopies(Renderer &renderer) : renderer_(renderer)
{
}

void FrameCopies::create()
{
    argon::gfx::Presenter &presenter = renderer_.presenter();
    transition_ = presenter.createFrameCopy();
    lens_ = presenter.createFrameCopy();
    menuFeedback_ = presenter.createFrameCopy();
}

void FrameCopies::destroy()
{
    argon::gfx::Presenter &presenter = renderer_.presenter();
    presenter.destroyFrameCopy(menuFeedback_);
    presenter.destroyFrameCopy(transition_);
    presenter.destroyFrameCopy(lens_);
}

// the lens copy is bound to every scene set when made again
void FrameCopies::fitToWindow()
{
    argon::gfx::Presenter &presenter = renderer_.presenter();
    const bool transitionResized = !presenter.frameCopyFits(transition_);
    if (presenter.fitFrameCopy(transition_) && transitionResized)
    {
        frameKept_ = false;
        transitionShowing_ = false;
    }
    const bool lensResized = !presenter.frameCopyFits(lens_);
    if (presenter.fitFrameCopy(lens_) && lensResized)
    {
        renderer_.bindFrameCopy(presenter.frameCopyTexture(lens_));
    }
}

void FrameCopies::startTransition()
{
    if (!frameKept_)
    {
        return;
    }
    transitionShowing_ = true;
    transitionStart_ = glfwGetTime();
}

// 0xa72a0: the kept frame full screen at alpha 255 - 255 x clamp((now - start) / duration), off after
void FrameCopies::drawTransition(ImDrawList *drawList)
{
    const ImTextureID image = renderer_.presenter().frameCopyImage(transition_);
    if (!transitionShowing_ || !image)
    {
        return;
    }
    const float progress = float((glfwGetTime() - transitionStart_) / kTransitionSeconds);
    if (progress >= 1.f)
    {
        transitionShowing_ = false;
        return;
    }
    const int alpha = 255 - std::clamp(int(progress * 255.f), 0, 255);
    drawList->AddImage(image, ImVec2(0, 0), ImGui::GetIO().DisplaySize, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, alpha));
}

void FrameCopies::keepFinishedFrame()
{
    if (transitionShowing_ || !renderer_.presenter().frameCopyFits(transition_))
    {
        return;
    }
    renderer_.presenter().copyFinishedFrame(transition_);
    frameKept_ = true;
}

// the race scene copies the back buffer into the echo after the 3D view (0x93aa8) and only then draws the sorted
// see-through bucket (0x93c49), which holds the Hud Lens meshes
void FrameCopies::copySceneForLenses()
{
    if (renderer_.presenter().frameCopyFits(lens_))
    {
        renderer_.presenter().copySceneSoFar(lens_);
    }
}

// window sized, the swapchain's format (copied straight from the finished background)
bool FrameCopies::fitMenuFeedback()
{
    return renderer_.presenter().fitFrameCopy(menuFeedback_);
}

ImTextureID FrameCopies::menuFeedbackImage() const
{
    return renderer_.presenter().frameCopyImage(menuFeedback_);
}

void FrameCopies::cutToMenuFeedback(ImDrawList *drawList)
{
    drawList->AddCallback(cutCallback, this);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

// inside ImGui's draw: end the pass (the frame so far is in the swapchain image), copy it into the feedback copy,
// start the pass again (cleared); ImGui then draws the copy under the rest
void FrameCopies::cutCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    auto *copies = static_cast<FrameCopies *>(cmd->UserCallbackData);
    argon::gfx::Presenter &presenter = copies->renderer_.presenter();
    if (presenter.frameCopyFits(copies->menuFeedback_))
    {
        presenter.copySceneAndRestart(copies->menuFeedback_, argon::gfx::SceneClear{argon::gfx::Colour{0, 0, 0, 1}, argon::kDepthClear});
    }
}
