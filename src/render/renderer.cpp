// Argon: the GPU side a run keeps from start to shutdown (render\renderer.hpp).
#include "render/renderer.hpp"

#include "gfx/vulkan/vulkan_handles.hpp"
#include "render/push_constants.hpp"

#include <cstdio>
#include <stdexcept>

namespace
{

argon::TextureData whitePixel()
{
    argon::TextureData pixel;
    pixel.width = pixel.height = 1;
    pixel.format = argon::gfx::TextureFormat::Rgba8;
    pixel.bytes = {255, 255, 255, 255};
    pixel.levels.push_back({1, 1, 0, 4});
    return pixel;
}

argon::TextureData greyCube()
{
    argon::TextureData cube = whitePixel();
    cube.layers = 6;
    cube.cube = true;
    cube.layerStride = 4;
    cube.bytes.assign(24, 160);
    return cube;
}

}  // namespace

void Renderer::start(const argon::gfx::BackendOptions &options, GLFWwindow *window)
{
    backend_ = argon::gfx::createBackend(options, window, uint32_t(sizeof(PushConstants)));
    kind_ = options.kind;
}

void Renderer::startOrFallBack(const argon::gfx::BackendOptions &options, GLFWwindow *window)
{
#ifdef _WIN32
    if (options.kind == argon::gfx::BackendKind::Vulkan)
    {
        std::string vulkanError;
        try
        {
            start(options, window);
            return;
        }
        catch (const std::exception &error)
        {
            vulkanError = error.what();
            std::fprintf(stderr, "renderer: vulkan: %s; trying d3d12\n", error.what());
        }
        argon::gfx::BackendOptions direct3D = options;
        direct3D.kind = argon::gfx::BackendKind::Direct3D12;
        try
        {
            start(direct3D, window);
        }
        catch (const std::exception &error)
        {
            throw std::runtime_error("no renderer could start. Vulkan: " + vulkanError + ". Direct3D 12: " + error.what());
        }
        return;
    }
#endif
    start(options, window);
}

void Renderer::createResources(const std::string &gameFolder, int viewSlots)
{
    createLineAndWaterPipelines();
    createSetPools();
    createFallbackTextures();
    meshDraws.create(device(), argon::gfx::kFramesInFlight);
    sceneBuffers.create(device(), white, viewSlots, meshDraws.buffers());
    shadows.create(device(), white, propPool);
    effectBuffers.create(device(), white, propPool, gameFolder);
    flares.create(device());
    postAntiAliasing = std::make_unique<PostAntiAliasingPass>(device(), presenter());
    translucency = std::make_unique<OrderIndependentTranslucency>(device(), presenter(), sceneBuffers.sets());
}

// the editor's lines (the depth test is dynamic: "on top" uses the same pipeline) and the game's water: blend mode 1
// (0xa9530: SRC_ALPHA / ONE_MINUS_SRC_ALPHA, add) with the device's LEQUAL depth test (0xa69b3; docs\water\README.md "Render states")
void Renderer::createLineAndWaterPipelines()
{
    linePipe = device().createPipeline({.program = argon::gfx::ShaderProgram::Line, .topology = argon::gfx::Topology::Lines});
    const argon::MeshState alpha = argon::MeshState::alphaBlended();
    waterPipe = device().createPipeline({.program = argon::gfx::ShaderProgram::Water, .blend = &alpha});
}

void Renderer::createFallbackTextures()
{
    white = device().createTexture(whitePixel());
    whiteCube = device().createTexture(greyCube());
}

void Renderer::createSetPools()
{
    trackPool = device().createSetPool({4096, 8});
    spareTrackPool = device().createSetPool({4096, 8});
    propPool = device().createSetPool({4096, 128});
}

void Renderer::releaseScreenPasses()
{
    postAntiAliasing->destroy();
}

void Renderer::startUi()
{
    backend_->startUi();
}

void Renderer::stopUi()
{
    backend_->stopUi();
}

void Renderer::destroy()
{
    device().destroyTexture(white);
    device().destroyTexture(whiteCube);
    sceneBuffers.destroy(device());
    meshDraws.destroy(device());
    shadows.destroy();
    effectBuffers.destroy();
    flares.destroy();
    backend_->destroy();
    backend_.reset();
}

bool Renderer::started() const
{
    return backend_ != nullptr;
}

argon::gfx::BackendKind Renderer::kind() const
{
    return kind_;
}

argon::gfx::Device &Renderer::device() const
{
    return backend_->device();
}

argon::gfx::Presenter &Renderer::presenter() const
{
    return backend_->presenter();
}

argon::gfx::UiRenderer &Renderer::ui() const
{
    return backend_->ui();
}

void Renderer::bindFrameCopy(argon::gfx::TextureRef copy)
{
    for (const argon::gfx::SceneSet set : sceneBuffers.sets())
    {
        device().useFrameCopy(set, copy);
    }
}

void Renderer::beginUploadBatch()
{
    device().beginUploadBatch();
}

void Renderer::submitUploadBatch()
{
    device().submitUploadBatch();
}

void Renderer::beginFrame(argon::gfx::CommandList &commands, int frame)
{
    meshDraws.begin(frame);
    flares.readResults(commands, frame, int(presenter().samples()));
}

void Renderer::writeSceneConstants(int viewSlot, const SceneUbo &constants)
{
    sceneBuffers.write(viewSlot, constants);
}

argon::gfx::SceneSet Renderer::sceneSet(int viewSlot) const
{
    return sceneBuffers.set(viewSlot);
}

void Renderer::recordShadows(argon::gfx::CommandList &commands, const std::vector<ShadowedShip> &ships, const ShadowView &view)
{
    shadows.record(commands, ships, ShipShadows::Frame{view.frame, view.sun, view.receivers, sceneSet(view.viewSlot), meshDraws, view.solid});
}

void Renderer::clearShadows()
{
    shadows.clear();
}

void Renderer::writeTrails(int frame, const argon::EngineTrails &trails)
{
    effectBuffers.writeTrails(frame, trails);
}

void Renderer::clearTrails()
{
    effectBuffers.clearTrails();
}

void Renderer::prepareScreenPasses(PostAntiAliasing postAntiAliasingMode, bool orderIndependent)
{
    postAntiAliasing->prepare(postAntiAliasingMode);
    translucency->prepare(orderIndependent);
}

void Renderer::recordPostAntiAliasing(argon::gfx::CommandList &commands)
{
    postAntiAliasing->record(commands);
}

void Renderer::loadParticleTextures()
{
    effectBuffers.loadParticleTextures();
}

bool Renderer::hasSprayTexture(const std::string &track) const
{
    return effectBuffers.sprayTextureOf(track);
}

void Renderer::useSprayTexture(const argon::TextureData &picture, const std::string &track)
{
    effectBuffers.useSprayTexture(picture, track);
}
