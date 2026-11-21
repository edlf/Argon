#version 450
// SMAA pass 2: the blending weights from the edges (tex[0]) with the area (tex[1]) and search (tex[2]) tables
#include "smaa_common.glsl"

void main() {
    vec2 pixel;
    vec4 offset[3];
    SMAABlendingWeightCalculationVS(vUV, pixel, offset);
    outColour = SMAABlendingWeightCalculationPS(vUV, pixel, offset, tex[0], tex[1], tex[2], vec4(0.0));
}
