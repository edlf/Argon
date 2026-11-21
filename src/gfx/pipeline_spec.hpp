// Argon: what a pipeline is, the same for every backend: its shader program, the primitives, the fill, the blend state
// (the mesh state's), the depth compare and what it draws into. Depth test / write, culling and the front face are set
// per draw (gfx::CommandList). Each backend keeps the programs' compiled shaders (SPIR-V for Vulkan).
#pragma once
#include "model/material.hpp"

namespace argon::gfx
{

// a vertex and fragment shader pair (shaders\*.vert / *.frag)
enum class ShaderProgram
{
    Mesh,            // mesh.vert / mesh.frag: the models
    Line,            // line: the editor's markings
    Water,           // water: the track's water (water_draw 0xce1c0)
    Trail,           // trail.vert / trail.frag: the ship trails and the particles
    Spray,           // trail.vert / spray.frag: the spray and the trail sprites
    QuadSprite,      // quadsprite: sprite.dat's quads
    Occlusion,       // occl: the flare occlusion points
    ShadowCast,      // mesh.vert / shadowcast.frag: a ship into its shadow map
    ShadowSolid,     // mesh.vert / shadowsolid.frag: the solid shadow
    ShadowReceive,   // shadow.vert / shadow.frag: the shadows on the track
    Fxaa,            // fullscreen.vert / fxaa.frag: the post-process FXAA (render\post_anti_aliasing.hpp)
    SmaaEdges,       // fullscreen.vert / smaa_edges.frag: SMAA's edge detection
    SmaaWeights,     // fullscreen.vert / smaa_weights.frag: SMAA's blending weights
    SmaaBlend,       // fullscreen.vert / smaa_blend.frag: SMAA's neighbourhood blending
    MeshOit,         // mesh.vert / mesh_oit.frag: a translucent mesh into the OIT targets (RenderTarget::Oit)
    MeshOitOpaque,   // mesh.vert / mesh_oit_opaque.frag: a translucent mesh's solid pixels, before the OIT pass
    OitComposite,    // fullscreen.vert / oit_composite(_ms).frag: the OIT targets over the scene (the _ms one with MSAA)
};

// the programs that draw one triangle over the whole target from gl_VertexIndex alone (no vertex buffer)
inline bool isFullscreenProgram(ShaderProgram program)
{
    return program == ShaderProgram::Fxaa || program == ShaderProgram::SmaaEdges || program == ShaderProgram::SmaaWeights ||
           program == ShaderProgram::SmaaBlend || program == ShaderProgram::OitComposite;
}

// the Vertex attributes (locations 0 position, 1 normal, 2 / 3 the UV pairs, 4 colour) a triangle program's vertex
// shader reads, a bit per location; a pipeline's vertex input holds only these (the line programs read both of
// theirs)
inline uint32_t vertexInputsOf(ShaderProgram program)
{
    if (isFullscreenProgram(program))
    {
        return 0u;
    }
    if (program == ShaderProgram::Occlusion)
    {
        return 0b00001u;   // occl.vert: the position
    }
    return program == ShaderProgram::QuadSprite ? 0b10101u : 0b11111u;   // quadsprite.vert: position, UV pair 0, colour
}

enum class Topology
{
    Triangles,
    Lines,
    OcclusionPoints   // points with the colour writes off (the flare test)
};

enum class Fill
{
    Solid,
    Wireframe
};

enum class DepthCompare
{
    NearerOrEqual,   // the scene's reversed depth (argon::kDepthNearerOrEqual)
    Always
};

enum class RenderTarget
{
    Scene,    // the window's scene pass, with its MSAA
    Layer,    // a layer of a LayerTarget (a ship's shadow map, the SMAA passes)
    Oit       // the OIT pass (gfx::Presenter::beginOitPass): its two targets with their fixed blends, the scene's depth
};

// the OIT targets' blends: the accumulated colour adds (ONE / ONE), the revealage multiplies by 1 - alpha (ZERO /
// ONE_MINUS_SRC_COLOR); a pipeline for RenderTarget::Oit takes these whatever its spec's blend
inline constexpr uint32_t kOitTargetCount = 2;

struct PipelineSpec
{
    ShaderProgram program = ShaderProgram::Mesh;
    Topology topology = Topology::Triangles;
    Fill fill = Fill::Solid;
    const MeshState *blend = nullptr;   // none: opaque
    DepthCompare depth = DepthCompare::NearerOrEqual;
    RenderTarget target = RenderTarget::Scene;
};

}  // namespace argon::gfx
