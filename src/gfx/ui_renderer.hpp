// Argon: ImGui drawn by the backend, the same for every backend: its fonts, the textures the HUD and the menus show as
// ImGui images, the frame's ImGui draw data in the scene pass, and the HUD's own blends switched inside it from ImGui
// draw callbacks. A backend implements it over its ImGui renderer backend (gfx\vulkan\vulkan_ui_renderer.hpp).
#pragma once
#include "gfx/texture.hpp"

#include <imgui.h>

namespace argon::gfx
{

// the HUD's blends (the game's blend modes 1 and 2, and the negative brightness)
enum class UiBlend
{
    Additive,      // ONE / ONE, added (mode 2)
    Subtractive,   // the framebuffer minus the colour (negative brightness)
    Panel          // the plasma panel shader over a 2D array image, alpha blended (mode 1)
};

class UiRenderer
{
  public:
    virtual ~UiRenderer() = default;

    // before ImGui::NewFrame
    virtual void newFrame() = 0;
    // the frame's ImGui draw data (ImGui::Render done), into the presenter's scene pass
    virtual void render() = 0;
    // the texture's first layer (all its levels) as an ImGui image, kept until removeImage
    virtual ImTextureID addImage(const Texture &texture, SamplerMode sampling) = 0;
    // all its layers, for the UiBlend::Panel draws
    virtual ImTextureID addLayeredImage(const Texture &texture, SamplerMode sampling) = 0;
    // before its texture is destroyed; none: nothing
    virtual void removeImage(ImTextureID image) = 0;
    // inside render (an ImGui draw callback): the draws after it blend so, until ImDrawCallback_ResetRenderState
    virtual void useBlend(UiBlend blend) = 0;
    // the window was made again (Presenter::takeRemade)
    virtual void windowRemade() = 0;
};

}  // namespace argon::gfx
