#version 450
// sprite.dat's quads (the sprite pass 0xbd8f0 with quadsprite.xvu): the corners are placed on the CPU
// (track_sprites.cpp); oFog is the view depth (c32).
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec4 inUV01;
layout(location = 4) in vec4 inColour;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;
layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vCol;
layout(location = 2) out float vDepth;
void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    vUV = inUV01.xy;
    vCol = inColour;
    vDepth = gl_Position.w;
}
