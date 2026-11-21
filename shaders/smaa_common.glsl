// SMAA 1x (Jimenez et al., "SMAA: Enhanced Subpixel Morphological Antialiasing"; the reference SMAA.hlsl, MIT
// licence, fetched at configure time) ported to argon's bindings: the textures are set 0's sampler2DArray tex[4]
// (layer 0), the target's metrics the push constants. Included by smaa_edges / smaa_weights / smaa_blend.frag.
layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
layout(push_constant) uniform Push { vec4 rtMetrics; } pc;   // 1 / width, 1 / height, width, height

#define SMAA_RT_METRICS pc.rtMetrics
#define SMAA_PRESET_HIGH 1
#define SMAA_CUSTOM_SL 1

#define SMAATexture2D(tex) sampler2DArray tex
#define SMAATexturePass2D(tex) tex
#define SMAASampleLevelZero(tex, coord) textureLod(tex, vec3(coord, 0.0), 0.0)
#define SMAASampleLevelZeroPoint(tex, coord) textureLod(tex, vec3(coord, 0.0), 0.0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) textureLodOffset(tex, vec3(coord, 0.0), 0.0, offset)
#define SMAASample(tex, coord) texture(tex, vec3(coord, 0.0))
#define SMAASamplePoint(tex, coord) texture(tex, vec3(coord, 0.0))
#define SMAASampleOffset(tex, coord, offset) textureOffset(tex, vec3(coord, 0.0), offset)
#define SMAAGather(tex, coord) textureGather(tex, vec3(coord, 0.0))
#define SMAA_FLATTEN
#define SMAA_BRANCH
#define lerp(a, b, t) mix(a, b, t)
#define saturate(a) clamp(a, 0.0, 1.0)
#define mad(a, b, c) fma(a, b, c)
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4

#include "SMAA.hlsl"

layout(location = 0) out vec4 outColour;
