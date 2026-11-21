// Argon: the shaders' SPIR-V (shaders\*.vert / *.frag, compiled by glslc at build time into the build's shaders
// folder as number lists).
#pragma once
#include <cstdint>

inline const uint32_t kMeshVert[] = {
#include "mesh.vert.inc"
};
inline const uint32_t kMeshFrag[] = {
#include "mesh.frag.inc"
};
inline const uint32_t kLineVert[] = {
#include "line.vert.inc"
};
inline const uint32_t kLineFrag[] = {
#include "line.frag.inc"
};
inline const uint32_t kWaterVert[] = {
#include "water.vert.inc"
};
inline const uint32_t kWaterFrag[] = {
#include "water.frag.inc"
};
inline const uint32_t kShadowVert[] = {
#include "shadow.vert.inc"
};
inline const uint32_t kShadowFrag[] = {
#include "shadow.frag.inc"
};
inline const uint32_t kShadowCastFrag[] = {
#include "shadowcast.frag.inc"
};
inline const uint32_t kShadowSolidFrag[] = {
#include "shadowsolid.frag.inc"
};
inline const uint32_t kHudAddVert[] = {
#include "hudadd.vert.inc"
};
inline const uint32_t kHudPanelFrag[] = {
#include "hudpanel.frag.inc"
};
inline const uint32_t kHudAddFrag[] = {
#include "hudadd.frag.inc"
};
inline const uint32_t kSprayFrag[] = {
#include "spray.frag.inc"
};
inline const uint32_t kQuadSpriteVert[] = {
#include "quadsprite.vert.inc"
};
inline const uint32_t kQuadSpriteFrag[] = {
#include "quadsprite.frag.inc"
};
inline const uint32_t kTrailVert[] = {
#include "trail.vert.inc"
};
inline const uint32_t kTrailFrag[] = {
#include "trail.frag.inc"
};
inline const uint32_t kOcclVert[] = {
#include "occl.vert.inc"
};
inline const uint32_t kOcclFrag[] = {
#include "occl.frag.inc"
};
inline const uint32_t kFullscreenVert[] = {
#include "fullscreen.vert.inc"
};
inline const uint32_t kFxaaFrag[] = {
#include "fxaa.frag.inc"
};
inline const uint32_t kSmaaEdgesFrag[] = {
#include "smaa_edges.frag.inc"
};
inline const uint32_t kSmaaWeightsFrag[] = {
#include "smaa_weights.frag.inc"
};
inline const uint32_t kSmaaBlendFrag[] = {
#include "smaa_blend.frag.inc"
};
inline const uint32_t kMeshOitFrag[] = {
#include "mesh_oit.frag.inc"
};
inline const uint32_t kMeshOitOpaqueFrag[] = {
#include "mesh_oit_opaque.frag.inc"
};
inline const uint32_t kOitCompositeFrag[] = {
#include "oit_composite.frag.inc"
};
inline const uint32_t kOitCompositeMsFrag[] = {
#include "oit_composite_ms.frag.inc"
};
