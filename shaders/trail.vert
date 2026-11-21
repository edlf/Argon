#version 450
// Ship trail ribbons (trail.cpp, draw 0x9dca0): world-space points, vertex colour, two uv sets whose v is
// scrolled by the texture transforms (stage 0 by -0.5, stage 1 by -2 per game frame).
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec4 inUV01;
layout(location = 3) in vec4 inUV23;
layout(location = 4) in vec4 inColour;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;   // model[0].xy = v scroll of stage 0 / 1
layout(location = 0) out vec2 vUV0;
layout(location = 1) out vec2 vUV1;
layout(location = 2) out vec4 vCol;
void main() {
    gl_Position = pc.mvp * vec4(inPos + 1e-30 * (inNrm + inUV23.xyz), 1.0);
    vUV0 = inUV01.xy + vec2(0.0, pc.model[0].x);
    vUV1 = inUV01.zw + vec2(0.0, pc.model[0].y);
    vCol = inColour;
}
