#version 450
// Projected ship shadow receivers (0x9b820): ground triangles near a ship with the shadow-map uv,
// the shadow layer and the game's height fade as vertex colour.
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec4 inUV01;    // xy = shadow-map uv, z = shadow layer
layout(location = 3) in vec4 inUV23;
layout(location = 4) in vec4 inColour;  // grey = clamp(35 - |dy|, 0, 30) / 255
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;
layout(location = 0) out vec3 vUV;
layout(location = 1) out vec3 vCol;
void main() {
    gl_Position = pc.mvp * vec4(inPos + 1e-30 * (inNrm + inUV23.xyz), 1.0);
    vUV = inUV01.xyz;
    vCol = inColour.rgb;
}
