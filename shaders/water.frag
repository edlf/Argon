#version 450
// Port of the game's water pixel shader (D3DPIXELSHADERDEF at 0xa1ee0, see docs/WATER.md):
// t0 = bump map, t1..t3 = texm3x3vspec (N = [T B N] * (t0 * 2 - 1), reflect the eye vector, cube lookup),
// final rgb = t3 * v0.rgb, alpha = v0.a (Fresnel); no fog (0xa8450 mode 0). The cube maps are 6-layer arrays (+X -X +Y -Y +Z -Z).
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vEye;
layout(location = 2) in vec3 vT;
layout(location = 3) in vec3 vB;
layout(location = 4) in vec3 vN;
layout(location = 5) in vec4 vColour;
layout(location = 7) flat in vec2 vSel;

layout(set = 0, binding = 0) uniform sampler2DArray tex[4];   // 0 bump frames, 1..3 cube slots 0..2 (default, waterref1, 2)
layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;
    ivec4 toggles, toggles2;
} scene;

layout(location = 0) out vec4 outColour;

vec3 cubeLookup(float slot, vec3 r) {
    vec3 a = abs(r);
    float face, ma, sc, tc;
    if (a.x >= a.y && a.x >= a.z) { face = r.x > 0.0 ? 0.0 : 1.0; ma = a.x; sc = r.x > 0.0 ? -r.z : r.z; tc = -r.y; }
    else if (a.y >= a.z)          { face = r.y > 0.0 ? 2.0 : 3.0; ma = a.y; sc = r.x; tc = r.y > 0.0 ? r.z : -r.z; }
    else                          { face = r.z > 0.0 ? 4.0 : 5.0; ma = a.z; sc = r.z > 0.0 ? r.x : -r.x; tc = -r.y; }
    vec3 uvw = vec3((vec2(sc, tc) / ma + 1.0) * 0.5, face);
    if (slot > 1.5) return textureLod(tex[3], uvw, 0.0).rgb;
    return slot > 0.5 ? textureLod(tex[2], uvw, 0.0).rgb : textureLod(tex[1], uvw, 0.0).rgb;
}

void main() {
    vec3 n = texture(tex[0], vec3(vUV, vSel.x)).rgb * 2.0 - 1.0;   // PSDotMapping MINUS1_TO_1
    vec3 N = vT * n.x + vB * n.y + vN * n.z;
    vec3 E = vEye;
    vec3 R = 2.0 * N * dot(N, E) / max(dot(N, N), 1e-6) - E;        // DOT_RFLCT_SPEC
    vec3 rgb = cubeLookup(vSel.y, R) * vColour.rgb;
    float alpha = clamp(vColour.a, 0.0, 1.0);

    outColour = vec4(rgb, alpha);
}
