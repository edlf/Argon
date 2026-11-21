#version 450
// sprite.dat's quads (the sprite pass 0xbd8f0): quadsprite.dds x vertex colour with the track fog; list A fogs to
// the fog colour, list B (additive) to black (0xa8610 sets the fog colour 0).
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vCol;
layout(location = 2) in float vDepth;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;   // model[0].z: alpha from the vertex only, .w: additive
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;
    ivec4 toggles, toggles2;
} scene;
layout(location = 0) out vec4 outColour;

float fogFactor() {
    if (scene.toggles.x == 0)         return 1.0;
    if (scene.fogParams.x < 1.5)      return exp(-scene.fogParams.y * vDepth);
    if (scene.fogParams.x < 2.5)      return exp(-pow(scene.fogParams.y * vDepth, 2.0));
    return clamp(1.0 - vDepth / max(scene.fogParams.z, 1.0), 0.0, 1.0);
}

void main() {
    vec4 t = texture(tex[0], vec3(vUV, 0.0));
    // the stage combine is inherited from earlier draws: alpha either texture x vertex or vertex only
    float a = pc.model[0].z > 0.5 ? vCol.a : t.a * vCol.a;
    vec3 rgb = t.rgb * vCol.rgb;
    float f = fogFactor();
    rgb = pc.model[0].w > 0.5 ? rgb * f : mix(scene.fogColour.rgb, rgb, f);
    outColour = vec4(rgb, a);
}
