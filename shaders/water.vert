#version 450
// Port of the game's shaders\waterbumpfresnal.xvu (see docs/WATER.md). The grid vertex index picks the
// vertex in the two precomputed wave frames (set 2), which are blended like c8 = (w, 1 - w).
layout(location = 0) in vec3 inPos;   // grid vertex (unused: the wave buffer holds the data)
layout(location = 1) in vec3 inNrm;   // (shared vertex layout, unused)
layout(location = 2) in vec4 inUV01;
layout(location = 3) in vec4 inUV23;
layout(location = 4) in vec4 inColour;

layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;
    ivec4 toggles, toggles2;
} scene;

// per vertex and frame: position (local), tangent, binormal, basis normal (0xcd170); the w of the first three hold the
// Fresnel normal (stream 2, 0xcd4f0)
layout(std430, set = 2, binding = 0) readonly buffer Wave { vec4 v[]; } wave;

// model[0] = (frame a, frame b, weight w, bump frame), model[1].x = cube map, model[3] = patch offset (c5)
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;

layout(location = 0) out vec2 vUV;          // oT0: bump map
layout(location = 1) out vec3 vEye;         // oT1.w..oT3.w: to the eye
layout(location = 2) out vec3 vT;           // basis (oT1..oT3 = rows of [T B N])
layout(location = 3) out vec3 vB;
layout(location = 4) out vec3 vN;
layout(location = 5) out vec4 vColour;      // oD0
layout(location = 7) flat out vec2 vSel;    // bump frame, cube map

void main() {
    int idx = gl_VertexIndex;
    int a = int(pc.model[0].x), b = int(pc.model[0].y);
    float w = pc.model[0].z;
    int oa = (a * 289 + idx) * 4, ob = (b * 289 + idx) * 4;
    vec3 p = wave.v[oa].xyz * (1.0 - w) + wave.v[ob].xyz * w;
    vec3 world = p + pc.model[3].xyz + 1e-30 * (inPos + inNrm + inUV01.xyz + inUV23.xyz + inColour.rgb);
    gl_Position = pc.mvp * vec4(world, 1.0);
    vUV = 0.0625 * p.xz;
    vEye = scene.camTime.xyz - world;
    vT = wave.v[oa + 1].xyz * (1.0 - w) + wave.v[ob + 1].xyz * w;
    vB = wave.v[oa + 2].xyz * (1.0 - w) + wave.v[ob + 2].xyz * w;
    vN = wave.v[oa + 3].xyz * (1.0 - w) + wave.v[ob + 3].xyz * w;
    // Fresnel from frame a's normal (stream 2 = v8): c9.w + (1 - c9.w) * (1 - dot(E, n)), colour c9.rgb
    vec3 fresnelNormal = vec3(wave.v[oa].w, wave.v[oa + 1].w, wave.v[oa + 2].w);
    float d = dot(normalize(vEye), fresnelNormal);
    vColour = vec4(0.5, 0.7, 1.0, 0.6 + 0.4 * (1.0 - d));
    vSel = vec2(pc.model[0].w, pc.model[1].x);
}
