// A texture that takes a new video frame now and then: the frame is written into the frame slot's upload buffer and
// copied into the texture by that frame's commands (recorded after its fence wait, before its scene pass). Shown as
// an ImGui image.
#pragma once
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "gfx/ui_renderer.hpp"

struct VideoSize
{
    uint32_t width = 0, height = 0;
    bool operator==(const VideoSize &) const = default;
};

class VideoTexture
{
  public:
    // black until the first upload; the GPU done with the one it replaces
    void create(argon::gfx::Device &device, argon::gfx::UiRenderer &ui, const VideoSize &size);
    void destroy();
    bool ready() const
    {
        return texture_.valid;
    }
    VideoSize size() const
    {
        return size_;
    }
    ImTextureID image() const
    {
        return image_;
    }
    // size().width x height x 4 bytes (B, G, R, 255), top-down; frameSlot's previous upload done on the GPU
    void upload(argon::gfx::CommandList &commands, int frameSlot, const uint8_t *bgrx);

  private:
    argon::gfx::Device *device_ = nullptr;
    argon::gfx::UiRenderer *ui_ = nullptr;
    VideoSize size_;
    argon::gfx::Texture texture_;
    ImTextureID image_{};
    argon::gfx::BufferRef staging_[argon::gfx::kFramesInFlight];
};
