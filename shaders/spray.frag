#version 450
// Water spray puffs (0x9d620 / 0x9c540): quadsprite.dds atlas cell x vertex colour (white, fading alpha),
// alpha-blended, with the track fog.
layout(location = 0) in vec2 vUV0;
layout(location = 1) in vec2 vUV1;
layout(location = 2) in vec4 vCol;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;   // model[0].z: 1 = alpha from the vertex only
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;
    ivec4 toggles, toggles2;
} scene;
layout(location = 0) out vec4 outColour;
void main() {
    vec4 t = texture(tex[0], vec3(vUV0, 0.0));
    // the sprite pass (0xbd8f0) leaves the stage combine to earlier state: alpha either texture x vertex or vertex only
    float a = pc.model[0].z > 0.5 ? vCol.a : t.a * vCol.a;
    outColour = vec4(t.rgb * vCol.rgb, a) + vec4(0.0 * vUV1, 0.0, 0.0);
}
