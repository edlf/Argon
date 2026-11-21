// Argon: the ship shadows (ship_shadows.hpp).
#include "render/ship_shadows.hpp"

#include "craft/drive.hpp"
#include "model/cmm.hpp"
#include "render/model_builder.hpp"
#include "render/push_constants.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <span>

using argon::gfx::SamplerMode;

namespace
{

// the ship shadow pass (0xbe230) draws with 0x50e327 clear, so cmm_draw_mesh (0xb6ea8) draws only the craft's
// shadow hull; a model without one casts with all its meshes but the effect parts
bool castsShipShadow(const Track &ship, const Draw &draw)
{
    if (ship.hasShadowHull)
    {
        return draw.shadowHull;
    }
    return !draw.fx;
}

}  // namespace

void ShipShadows::create(argon::gfx::Device &device, const Texture &white, argon::gfx::SetPool pool)
{
    device_ = &device;
    maps_ = device.createLayerTarget({uint32_t(kShadowSize), uint32_t(kShadowSize), uint32_t(kMaxShadows)});
    using argon::gfx::DepthCompare, argon::gfx::RenderTarget, argon::gfx::ShaderProgram;
    solidPipeline_ = device.createPipeline({.program = ShaderProgram::ShadowSolid, .depth = DepthCompare::Always, .target = RenderTarget::Layer});
    castPipeline_ = device.createPipeline({.program = ShaderProgram::ShadowCast, .depth = DepthCompare::Always, .target = RenderTarget::Layer});
    const argon::MeshState subtractive = argon::MeshState::subtractive();
    receivePipeline_ = device.createPipeline({.program = ShaderProgram::ShadowReceive, .blend = &subtractive});
    const Texture maps = device.layerTexture(maps_);
    StageTextures stages;
    stages.textures = {&maps, &white, &white, &white};
    stages.samplers = {SamplerMode::Clamp, SamplerMode::Wrap, SamplerMode::Wrap, SamplerMode::Wrap};
    mapSet_ = createStageSet(device, pool, stages);
    const size_t bytes = size_t(kMaxShadows) * kShadowTris * 3 * sizeof(argon::Vertex);
    for (size_t f = 0; f < buffers_.size(); ++f)
    {
        buffers_[f] = device.createStreamBuffer(argon::gfx::BufferUse::Vertex, bytes);
        mapped_[f] = device.mapped(buffers_[f]);
    }
}

void ShipShadows::destroy()
{
    for (auto &buffer : buffers_)
    {
        device_->destroyBuffer(buffer);
    }
}

void ShipShadows::record(argon::gfx::CommandList &commands, const std::vector<ShadowedShip> &ships, const Frame &frame)
{
    vertices_ = 0;
    const glm::vec3 sun = frame.sun;
    const glm::vec3 up = std::abs(sun.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    argon::Vertex *out = static_cast<argon::Vertex *>(mapped_[size_t(frame.frame)]);
    uint32_t layer = 0;
    for (const ShadowedShip &shadowed : ships)
    {
        if (layer >= uint32_t(kMaxShadows))
        {
            break;
        }
        const glm::vec3 pos(shadowed.placement[3]);
        const glm::mat4 lightViewProjection = glm::ortho(-6.75f, 6.75f, -6.75f, 6.75f, 0.001f, 3000.f) * glm::lookAt(pos - 100.f * sun, pos, up);
        const Caster caster{shadowed.ship, shadowed.placement, lightViewProjection, sun, layer};
        drawCaster(commands, caster, frame);
        collectReceivers(caster, frame.receivers, out);
        ++layer;
    }
}

void ShipShadows::clear()
{
    vertices_ = 0;
}

// the ship's silhouette into its layer, seen from the light
void ShipShadows::drawCaster(argon::gfx::CommandList &commands, const Caster &caster, const Frame &frame) const
{
    const Track &ship = *caster.ship;
    commands.beginLayerPass(maps_, caster.layer);
    commands.setFrontFace(argon::gfx::Winding::CounterClockwise);
    commands.setDepthCull({.test = false, .write = false, .cullBack = false});
    commands.bindPipeline(frame.solid ? solidPipeline_ : castPipeline_);
    commands.bindScene(frame.scene);
    commands.bindMaterials(ship.matSet);
    commands.bindVertexBuffer(ship.vb);
    commands.bindIndexBuffer(ship.ib);
    argon::gfx::TextureSet boundSet;
    for (const Draw &draw : ship.draws)
    {
        if (!castsShipShadow(ship, draw))
        {
            continue;
        }
        if (draw.texSet != boundSet)
        {
            commands.bindTextures(draw.texSet);
            boundSet = draw.texSet;
        }
        const PushConstants pc{caster.lightViewProjection * caster.model * draw.model, caster.model * draw.model};
        commands.pushConstants(std::as_bytes(std::span<const PushConstants, 1>(&pc, 1)));
        const uint32_t record = frame.meshDraws.add(MeshDraw{.material = int32_t(draw.matIndex)});
        commands.drawIndexed({uint32_t(draw.indexCount), uint32_t(draw.firstIndex), int32_t(draw.vertexOffset), record});
    }
    commands.endLayerPass();
}

// receivers: the ship box stretched 4 along the light, +-2 (0x9b820); up to kShadowTris light-facing collision
// triangles, their grey fading with the height difference
void ShipShadows::collectReceivers(const Caster &caster, const argon::CollisionGrid &grid, argon::Vertex *out)
{
    const glm::vec3 pos(caster.model[3]);
    const glm::vec3 light = caster.light;
    const glm::vec3 r(4.5f, 3.f, 4.5f);
    const glm::vec3 lo = glm::min(pos - r, pos - r + 4.f * light) - 2.f, hi = glm::max(pos + r, pos + r + 4.f * light) + 2.f;
    grid.trianglesInBox(lo, hi, triangles_);
    int used = 0;
    for (uint32_t ti : triangles_)
    {
        if (used >= kShadowTris)
        {
            break;
        }
        const glm::vec3 *triangle = grid.triangle(ti);
        glm::vec3 normal = glm::cross(triangle[2] - triangle[0], triangle[1] - triangle[0]);
        if (glm::length(normal) < 1e-8f)
        {
            continue;
        }
        normal = glm::normalize(normal);
        if (glm::dot(normal, light) >= 0.f)
        {
            continue;   // facing away from the light
        }
        for (int k = 0; k < 3; ++k)
        {
            argon::Vertex vertex{};
            const glm::vec3 raised = triangle[k] + normal * 0.03f;   // stands in for the game's z bias
            vertex.pos[0] = raised.x;
            vertex.pos[1] = raised.y;
            vertex.pos[2] = raised.z;
            const glm::vec4 clip = caster.lightViewProjection * glm::vec4(triangle[k], 1.f);
            vertex.uv[0][0] = clip.x * 0.5f + 0.5f;
            vertex.uv[0][1] = clip.y * 0.5f + 0.5f;
            vertex.uv[1][0] = float(caster.layer);
            const float grey = std::clamp(35.f - std::abs(triangle[k].y - pos.y), 0.f, 30.f);
            vertex.rgba[0] = vertex.rgba[1] = vertex.rgba[2] = uint8_t(grey);
            vertex.rgba[3] = 255;
            out[vertices_++] = vertex;
        }
        ++used;
    }
}

uint32_t ShipShadows::receiverVertices() const
{
    return vertices_;
}

argon::gfx::Pipeline ShipShadows::receiverPipeline() const
{
    return receivePipeline_;
}

argon::gfx::TextureSet ShipShadows::receiverTextures() const
{
    return mapSet_;
}

argon::gfx::BufferRef ShipShadows::receiverBuffer(int frame) const
{
    return buffers_[size_t(frame)];
}
