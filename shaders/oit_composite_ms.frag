#version 450
// the order-independent translucency's composite with MSAA: the targets have the scene's samples (oit_composite.glsl)
#define OIT_TARGET sampler2DMS
int oitSampleCount(sampler2DMS target) { return textureSamples(target); }
vec4 oitFetch(sampler2DMS target, ivec2 pixel, int sampleIndex) { return texelFetch(target, pixel, sampleIndex); }

#include "oit_composite.glsl"
