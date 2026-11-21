#version 450
// SMAA pass 1: the luma edges of the frame so far (tex[0]) into the edges target; pixels on no edge are discarded
#include "smaa_common.glsl"

void main() {
    vec4 offset[3];
    SMAAEdgeDetectionVS(vUV, offset);
    outColour = vec4(SMAALumaEdgeDetectionPS(vUV, offset, tex[0]), 0.0, 0.0);
}
