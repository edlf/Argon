#version 450
// Ship image for the shadow map. The game renders the ship with its own materials, lit with a white ambient,
// into a render target cleared to black (0xbe230), so glow planes only add their glow and the hull its colour.
// The viewer writes the stage-0 texture x vertex colour (alpha-tested cutouts discarded).
layout(location = 0) in vec3 vNrm;     // (declared to match mesh.vert)
layout(location = 1) in vec4 vUV01;
layout(location = 2) in vec4 vUV23;
layout(location = 3) in vec4 vColour;
layout(location = 4) in float vDepth;
layout(location = 5) in vec3 vRefl;
layout(location = 6) flat in int vMat;
layout(location = 7) in vec3 vWorld;
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
struct Stage { ivec4 op; vec4 anim; vec4 extra; };
struct Material { ivec4 hdr; vec4 params; Stage s[4]; };
layout(std430, set = 2, binding = 0) readonly buffer Materials { Material m[]; } mats;
layout(location = 0) out vec4 outColour;
void main() {
    Material M = mats.m[vMat];
    int set = M.s[0].op.w;
    vec2 uv = set == 0 ? vUV01.xy : set == 1 ? vUV01.zw : set == 2 ? vUV23.xy : vUV23.zw;
    vec4 t = M.hdr.x > 0 ? texture(tex[0], vec3(uv, 0.0)) : vec4(1.0);
    if (M.params.x > 0.0 && t.a < M.params.x) discard;
    vec3 c = M.hdr.y == 1 ? vColour.rgb : vec3(1.0);
    outColour = vec4(t.rgb * c, 1.0);
}
