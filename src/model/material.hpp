// Game render state for one mesh, decoded from its 48-byte surface block.
// Source: neon_setup_render_state (XBE 0x000a9530), FUN_000ba4f0 (0x000ba4f0)
// and the mesh draw FUN_000b6e60 (0x000b6e60).
#pragma once
#include <cstdint>

#include "gfx/blend.hpp"

namespace argon
{

struct MeshState
{
    bool blend = false;          // blend descriptor bit 0
    bool translucentPass = false;// bits 0+1: drawn in the second pass
    gfx::BlendFactor src = gfx::BlendFactor::One, dst = gfx::BlendFactor::Zero;
    gfx::BlendOp op = gfx::BlendOp::Add;
    float alphaRef = 0;          // discard if alpha < ref (0 = no test)
    bool depthWrite = true, depthTest = true, twoSided = false;
    // surface flag 0x20000: sorted last in the translucent bucket (0xb02d1) and lifted twice as far as the rest of it
    // (render\depth_lift.hpp)
    bool laidOver = false;
    // viewer only: opaque alpha-tested mesh drawn with alpha-to-coverage so MSAA smooths the cut edges
    bool a2c = false;

    static MeshState blended(gfx::BlendFactor srcFactor, gfx::BlendFactor dstFactor, gfx::BlendOp blendOp = gfx::BlendOp::Add)
    {
        MeshState meshState;
        meshState.blend = true;
        meshState.src = srcFactor;
        meshState.dst = dstFactor;
        meshState.op = blendOp;
        return meshState;
    }
    static MeshState alphaBlended()
    {
        return blended(gfx::BlendFactor::SrcAlpha, gfx::BlendFactor::OneMinusSrcAlpha);
    }
    static MeshState additive()
    {
        return blended(gfx::BlendFactor::One, gfx::BlendFactor::One);
    }
    // framebuffer - colour (the ship shadows)
    static MeshState subtractive()
    {
        return blended(gfx::BlendFactor::One, gfx::BlendFactor::One, gfx::BlendOp::ReverseSubtract);
    }

    uint32_t pipelineKey() const
    {   // what has to be baked into a pipeline
        return (blend ? (1u | uint32_t(src) << 1 | uint32_t(dst) << 6 | uint32_t(op) << 11) : 0u) | (a2c ? 1u << 21 : 0u);
    }
};

// which side of the blend a descriptor factor is for: an unknown code is ONE as the source, ZERO as the destination
enum class BlendSide
{
    Source,
    Destination
};

inline gfx::BlendFactor glFactor(uint32_t code, BlendSide side)
{
    // bit-coded factor from the descriptor (src: bits 2..13, dst: bits 16..27 >> 14)
    switch (code)
    {
    case 0x004:
        return gfx::BlendFactor::One;
    case 0x008:
        return gfx::BlendFactor::SrcColor;
    case 0x010:
        return gfx::BlendFactor::OneMinusSrcColor;
    case 0x020:
        return gfx::BlendFactor::SrcAlpha;
    case 0x040:
        return gfx::BlendFactor::OneMinusSrcAlpha;
    case 0x080:
        return gfx::BlendFactor::DstAlpha;
    case 0x100:
        return gfx::BlendFactor::OneMinusDstAlpha;
    case 0x200:
        return gfx::BlendFactor::DstColor;
    case 0x400:
        return gfx::BlendFactor::OneMinusDstColor;
    }
    return side == BlendSide::Source ? gfx::BlendFactor::One : gfx::BlendFactor::Zero;
}

// hasSurface = mesh flag 0x20; d = surface +0x08; f = surface +0x0c
inline MeshState decodeMeshState(bool hasSurface, uint32_t surfaceWord8, uint32_t surfaceWordC)
{
    MeshState meshState;
    if (!hasSurface)
    {
        return meshState;
    }
    meshState.depthTest = !(surfaceWordC & 0x10000);
    meshState.depthWrite = meshState.depthTest && !(surfaceWordC & 0x8);
    meshState.twoSided = (surfaceWordC & 0x10) != 0;
    meshState.laidOver = (surfaceWordC & 0x20000) != 0;
    if (surfaceWordC & 0x2000000)
    {
        meshState.alphaRef = 0.5f;            // alpha test, ref 0x80
    }
    if (!(surfaceWord8 & 1))
    {
        meshState.a2c = meshState.alphaRef > 0.f;
        return meshState;
    }   // blending off
    meshState.blend = true;
    // bits 0 + 1: the second pass. Its alpha test (ref 0) keeps alpha-0 texels in the game: the sun's FLARE_ texture
    // (Solar tex5) has a ring of alpha 0 with colour that its ONE / ONE draw adds in xemu
    meshState.translucentPass = (surfaceWord8 & 3) == 3;
    switch (surfaceWord8)
    {                                      // presets
    case 1:
        meshState.src = gfx::BlendFactor::SrcAlpha;
        meshState.dst = gfx::BlendFactor::OneMinusSrcAlpha;
        return meshState;
    case 2:
        meshState.src = meshState.dst = gfx::BlendFactor::One;
        return meshState;
    case 3:
        meshState.src = meshState.dst = gfx::BlendFactor::One;
        meshState.op = gfx::BlendOp::Subtract;
        return meshState;
    case 4:
        meshState.src = meshState.dst = gfx::BlendFactor::One;
        meshState.op = gfx::BlendOp::ReverseSubtract;
        return meshState;
    }
    uint32_t sc = surfaceWord8 & 0x3ffc, dc = (surfaceWord8 & 0xfff0000) >> 14, eq = surfaceWord8 & 0x70000000;
    bool known = (sc == 0x4 || sc == 0x8 || sc == 0x10 || sc == 0x20 || sc == 0x40 || sc == 0x80 || sc == 0x100 || sc == 0x200 || sc == 0x400) &&
                 (dc == 0x4 || dc == 0x8 || dc == 0x10 || dc == 0x20 || dc == 0x40 || dc == 0x80 || dc == 0x100 || dc == 0x200 || dc == 0x400);
    if (!known)
    {
        meshState.blend = false;
        return meshState;
    }        // game disables blending for unknown codes
    meshState.src = glFactor(sc, BlendSide::Source);
    meshState.dst = glFactor(dc, BlendSide::Destination);
    meshState.op = eq == 0x10000000   ? gfx::BlendOp::Subtract
           : eq == 0x20000000 ? gfx::BlendOp::ReverseSubtract
           : eq == 0x30000000 ? gfx::BlendOp::Min
           : eq == 0x40000000 ? gfx::BlendOp::Max
                              : gfx::BlendOp::Add;
    return meshState;
}

// ---------------------------------------------------------------- stages
// D3DTOP values (Xbox numbering) used by the shader combiner.
enum Top : int32_t
{
    TOP_DISABLE = 1,
    TOP_SELECTARG1,
    TOP_SELECTARG2,
    TOP_MODULATE,
    TOP_MODULATE2X,
    TOP_MODULATE4X,
    TOP_ADD,
    TOP_ADDSIGNED,
    TOP_ADDSIGNED2X,
    TOP_SUBTRACT,
    TOP_ADDSMOOTH,
    TOP_BLENDDIFFUSEALPHA,
    TOP_BLENDCURRENTALPHA,
    TOP_BLENDTEXTUREALPHA,
    TOP_BLENDFACTORALPHA,
    TOP_BLENDTEXTUREALPHAPM,
    TOP_PREMODULATE,
    TOP_MODALPHA_ADDCOLOR,
    TOP_MODCOLOR_ADDALPHA,
    TOP_MODINVALPHA_ADDCOLOR,
    TOP_MODINVCOLOR_ADDALPHA,
    TOP_DOTPRODUCT3
};

// Game combine code -> D3DTOP, from the jump tables in FUN_000a8cf0 (COLOROP, table 0x000a90a0)
// and FUN_000a9170 (ALPHAOP, table 0x000a9488). 0 / unknown = MODULATE, 0x17 = SELECTARG1, 999 = DISABLE.
// a texture stage's colour or alpha combine
enum class StageChannel
{
    Colour,
    Alpha
};

inline int32_t stageOp(uint32_t code, StageChannel channel)
{
    static const int32_t colour[19] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 13, 17, 18, 19, 20, 21, 4, 4, 22};
    static const int32_t alphaT[19] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 13, 17, 4, 4, 4, 4, 4, 4, 22};
    if (code == 999)
    {
        return TOP_DISABLE;
    }
    if (code == 0x17)
    {
        return TOP_SELECTARG1;
    }
    if (code >= 1 && code <= 19)
    {
        return channel == StageChannel::Alpha ? alphaT[code - 1] : colour[code - 1];
    }
    return TOP_MODULATE;
}

// std430 layout shared with mesh_shading.glsl (set 2, binding 0): one entry per draw.
struct GpuStage
{
    int32_t op[4];      // colour D3DTOP, alpha D3DTOP, stage flags, uv set
    float anim[4];      // rate (frames/s), frame count, scroll U per s, scroll V per s
    float extra[4];     // offset U, offset V, is bump map (normal map), 0
};
struct GpuMaterial
{
    int32_t hdr[4];     // stage count, has vertex colour, kind (0 normal, 1 sky, 2 additive, 3..8 the effects in
                        // mesh_shading.glsl: specular bump, its RGBA, diffuse specular bump, hud lens, color scale, spheremap),
                        // stage-0 texture bits (1 looks like cutout, 2 has alpha) for the fallback mode
    float params[4];    // game alpha-test reference, surface tint r g b (1 = none)
    float fog[4];       // the mesh's fog (render\mesh_fog.hpp): colour r g b, curve (-1 the track's, 0 off, 1 exp, 3 linear)
    float fogShape[4];  // its density, range (the mesh's own fog), 0, 0
    GpuStage stages[4];
};
static_assert(sizeof(GpuMaterial) == 256, "matches shader layout");

} // namespace argon
