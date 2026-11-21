#version 450
// the order-independent translucency's composite without MSAA (oit_composite.glsl)
#define OIT_TARGET sampler2D
int oitSampleCount(sampler2D target) { return 1; }
vec4 oitFetch(sampler2D target, ivec2 pixel, int sampleIndex) { return texelFetch(target, pixel, 0); }

#include "oit_composite.glsl"
