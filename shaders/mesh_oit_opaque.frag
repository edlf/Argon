#version 450
// an alpha-blended translucent mesh's solid pixels into the scene before the OIT pass (oit_split.glsl): drawn
// unblended with the depth written, so the OIT pass's translucent pixels behind them are hidden
layout(location = 0) out vec4 outColour;

#include "mesh_shading.glsl"
#include "oit_split.glsl"

void main() {
    shadeMesh();
    if (outColour.a < kOitOpaqueAlpha) discard;
}
