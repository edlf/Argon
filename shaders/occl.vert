#version 450
// flare occlusion test (0xa64f0): one 1-pixel point per sample, depth tested, no colour, inside an occlusion query
layout(location = 0) in vec3 inPos;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;
void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    gl_PointSize = 1.0;
}
