// Argon: a frame from start to the window, the same for every backend: wait for a frame slot, take the window's next
// image, record into the frame's command list (the scene pass in it), then submit and show it, keeping its picture
// when asked (the screenshot runs). The frame copies (the scene transition, the Hud Lens, the menus' feedback) are
// copies of the window's image, so the presenter makes and fills them too. A backend implements it over its swapchain (gfx\vulkan\vulkan_presenter.hpp).
#pragma once
#include "gfx/command_list.hpp"

#include <imgui.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace argon::gfx
{

inline constexpr int kFramesInFlight = 2;

// a finished frame's pixels as the window's images hold them: 4 bytes each (BGRA or RGBA), rows top first, packed
struct Picture
{
    uint32_t width = 0, height = 0;
    bool bgra = false;
    std::vector<uint8_t> pixels;
};

enum class FrameEnd
{
    Show,
    ShowAndKeep   // and keep its picture (takePicture)
};

class Presenter
{
  public:
    virtual ~Presenter() = default;

    // waits for the frame slot to be free and takes the window's next image; false when the window had to be made
    // again (resized, minimised): skip this frame
    virtual bool beginFrame() = 0;
    // the frame's commands, between beginFrame and endFrame
    virtual CommandList &commands() = 0;
    // the window's colour and depth cleared (the scene and the menus draw in it, ImGui last)
    virtual void beginScenePass(const SceneClear &clear) = 0;
    virtual void endScenePass() = 0;
    virtual void endFrame(FrameEnd end) = 0;
    // takePicture will not wait: the GPU has finished the kept frame, or none is kept (a window made again drops it)
    virtual bool pictureReady() const = 0;
    // the picture the last FrameEnd::ShowAndKeep kept (waiting for that frame alone); none when there is none
    virtual std::optional<Picture> takePicture() = 0;

    // a window-sized copy of the frame (the window's format, alpha read as 1), kept until destroyFrameCopy; holds
    // nothing until fitFrameCopy
    virtual FrameCopy createFrameCopy() = 0;
    virtual void destroyFrameCopy(FrameCopy &copy) = 0;
    // made again, black, when it is not the window's size (the GPU idle first); false when the window's images cannot
    // be copied and sampled
    virtual bool fitFrameCopy(FrameCopy copy) = 0;
    virtual bool frameCopyFits(FrameCopy copy) const = 0;
    // the copy for the scene sets (Device::useFrameCopy), until it is made again
    virtual TextureRef frameCopyTexture(FrameCopy copy) = 0;
    // the copy as an ImGui image (UiRenderer); none until it is fitted
    virtual ImTextureID frameCopyImage(FrameCopy copy) const = 0;
    // inside the scene pass: the frame so far into the copy and the pass left (layer passes may follow), until
    // resumeScenePass goes on over what it drew
    virtual void suspendScenePass(FrameCopy copy) = 0;
    virtual void resumeScenePass() = 0;
    // inside the scene pass: the frame so far into the copy, the pass going on over it
    void copySceneSoFar(FrameCopy copy)
    {
        suspendScenePass(copy);
        resumeScenePass();
    }
    // inside the scene pass: the frame so far into the copy, the pass started again cleared
    virtual void copySceneAndRestart(FrameCopy copy, const SceneClear &clear) = 0;
    // after the scene pass: the finished frame into the copy
    virtual void copyFinishedFrame(FrameCopy copy) = 0;

    // the order-independent translucency's targets made (true) or freed (false), the GPU idle first; while kept they
    // are made again with the window
    virtual void keepOitTargets(bool keep) = 0;
    // inside the scene pass, the targets kept: the scene pass left and the targets cleared, then drawn into over the
    // scene's depth (RenderTarget::Oit pipelines) until endOitPass goes on with the scene pass over what it drew
    virtual void beginOitPass() = 0;
    virtual void endOitPass() = 0;
    // the kept targets for the scene sets (Device::useOitTargets), and a number that changes each time they are made
    virtual OitTargets oitTargets() = 0;
    virtual uint64_t oitTargetsMade() const = 0;

    // the window's images: their size, and the samples per pixel of the scene pass (its MSAA)
    virtual WindowSize windowSize() const = 0;
    virtual uint32_t samples() const = 0;

    // the slot the next frame records into, 0 .. kFramesInFlight - 1
    virtual int frameSlot() const = 0;
    virtual uint64_t framesSubmitted() const = 0;
    // every submitted frame and upload done on the GPU
    virtual void waitForSubmittedWork() = 0;
    // the window was made again since the last call (its new image count, ImGui's)
    virtual bool takeRemade() = 0;
    // on: frames shown on the monitor's refresh; off: as soon as they are drawn (tearing where the window allows it).
    // A change may make the window's images again
    virtual void setVsync(bool on) = 0;
};

}  // namespace argon::gfx
