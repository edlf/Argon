// Argon: the post-process anti-aliasing over the 3D view (Argon Options' Post AA): FXAA, or SMAA 1x in three passes
// (the edges into one layer target, the blending weights into another, then the frame blended back over the scene).
// Recorded in the scene pass after the race views and before ImGui, so the HUD, the menus and their text stay sharp;
// the frame so far is read from a frame copy (Presenter::suspendScenePass). Neither mode is temporal: every frame
// stands alone, and nothing is drawn at another size.
#pragma once
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "gfx/texture.hpp"
#include "render/post_anti_aliasing_mode.hpp"

class PostAntiAliasingPass
{
  public:
    PostAntiAliasingPass(argon::gfx::Device &device, argon::gfx::Presenter &presenter);
    PostAntiAliasingPass(const PostAntiAliasingPass &) = delete;
    PostAntiAliasingPass &operator=(const PostAntiAliasingPass &) = delete;

    // what it made freed (the GPU done with it first)
    void destroy();
    // between frames: the frame copy, the SMAA targets and the texture sets fitted to the window for `mode` (waits for
    // the GPU when they have to be made again); Off frees the SMAA targets
    void prepare(PostAntiAliasing mode);
    // inside the scene pass, after the 3D views: the frame so far drawn back smoothed; nothing when prepare had nothing
    // to fit (Off, or the window's images cannot be copied)
    void record(argon::gfx::CommandList &commands);

  private:
    struct SmaaTargets
    {
        argon::gfx::LayerTarget edges, weights;
        argon::gfx::WindowSize size;
    };

    void createPipelines();
    void createLookupTables();
    bool fitSmaaTargets();
    void destroySmaaTargets();
    void makeTextureSets();
    argon::gfx::TextureSet textureSet(argon::gfx::TextureRef first, argon::gfx::TextureRef second, argon::gfx::TextureRef third);
    void drawOverTarget(argon::gfx::CommandList &commands, argon::gfx::Pipeline pipeline, argon::gfx::TextureSet textures) const;
    void recordSmaa(argon::gfx::CommandList &commands);

    argon::gfx::Device &device_;
    argon::gfx::Presenter &presenter_;
    argon::gfx::Pipeline fxaa_, smaaEdges_, smaaWeights_, smaaBlend_;
    argon::gfx::Texture areaTable_, searchTable_;
    argon::gfx::FrameCopy frame_;
    SmaaTargets smaa_;
    argon::gfx::SetPool sets_;
    argon::gfx::TextureSet frameSet_;     // the frame copy (FXAA, SMAA's edges)
    argon::gfx::TextureSet weightsSet_;   // SMAA's edges and tables
    argon::gfx::TextureSet blendSet_;     // the frame copy and SMAA's weights
    PostAntiAliasing mode_ = PostAntiAliasing::Off;   // what record draws (prepare's, Off when it fitted nothing)
};
