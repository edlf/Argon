#version 450
// SMAA pass 3: the frame so far (tex[0]) blended with its neighbours by the weights (tex[1]), back over the scene
#include "smaa_common.glsl"

void main() {
    vec4 offset;
    SMAANeighborhoodBlendingVS(vUV, offset);
    outColour = vec4(SMAANeighborhoodBlendingPS(vUV, offset, tex[0], tex[1]).rgb, 1.0);
}
