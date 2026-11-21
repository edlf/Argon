#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec4 inUV01;
layout(location = 3) in vec4 inUV23;
layout(location = 4) in vec4 inColour;

layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;
    ivec4 toggles, toggles2;
    vec4 light1Colour, light1Dir;
} scene;

layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;
#include "mesh_draws.glsl"

layout(location = 0) out vec3 vNrm;       // world normal
layout(location = 1) out vec4 vUV01;
layout(location = 2) out vec4 vUV23;
layout(location = 3) out vec4 vColour;
layout(location = 4) out float vDepth;
// env-map texcoord input. The game uses TCI_CAMERASPACEREFLECTIONVECTOR with the texture matrix 0x50e120 =
// transpose(view rotation) * translate(1,1,0) * scale(0.5,-0.5,1) (0xa9b60 / 0xa6d43), i.e. the world-space
// reflection vector, so the layer does not move when the camera only turns.
layout(location = 5) out vec3 vRefl;      // world-space reflection vector of the direction to the eye (effects' programs)
// the fixed-function texgen's: D3DRS_LOCALVIEWER is 0 for the whole game (0xa69a3), so the reflected eye vector is
// the camera's forward axis, the same for every vertex
layout(location = 13) out vec3 vReflFar;
layout(location = 6) flat out int vMat;   // the draw's material index (its record's)
layout(location = 15) flat out int vDraw; // the draw's record = firstInstance (mesh_draws.glsl)
// N.L of light 0 (the sun) and light 1 at the vertex: the fixed-function lighting (and the skinning programs) light
// per vertex and the colour is interpolated
layout(location = 9) out vec2 vNdl;
layout(location = 7) out vec3 vWorld;     // world position
layout(location = 8) flat out vec3 vTint; // bone tint x intensity of a mesh lit by its own colour: model row 3 when model[3][3] = 2
// "Diffuse Specular Bump Map" (material kind 5): the light and half vectors in the vertex's tangent basis
layout(location = 10) out vec3 vBumpLight;
layout(location = 11) out vec3 vBumpHalf;
// "Hud Lens" (material kind 6): the screen shift of the vertex moved 0.125 inwards along its normal (in uv units)
layout(location = 12) out vec2 vLensShift;
// "Spheremap Decal" (material kind 8): N.c5 at the vertex, c5 = the shell light's negated direction (0x39110c,
// unit(-0.5, -1, -1) from 0xaf360 / 0xaf3d0 / the init 0x76d60, the same on every page)
layout(location = 14) out float vShellNdl;
const vec3 kShellLightTowards = vec3(0.5, 1.0, 1.0) / 1.5;

// the effect's vertex program (stored in the model's effect record): the object-space light (c7 = -sun, normalised)
// and half vector (light + unit vector to the eye c10) dotted with the basis (src/render/bump_basis.hpp: tangent in
// the normal slot, binormal and the normal's side in UV sets 2 / 3) -> oT2 / oT3, normalised per pixel by the
// normalisation cube map on stages 2 / 3
void bumpVectors(vec3 worldPos) {
    mat3 toObject = transpose(mat3(pc.model));
    vec3 light = toObject * -scene.sunDir.xyz;
    vec3 toEye = toObject * (scene.camTime.xyz - worldPos);
    vec3 tangent = inNrm, binormal = vec3(inUV23.xy, inUV23.z);
    vec3 normal = inUV23.w * cross(tangent, binormal);
    vec3 halfway = light + normalize(toEye);
    vBumpLight = vec3(dot(light, tangent), dot(light, binormal), dot(light, normal));
    vBumpHalf = vec3(dot(halfway, tangent), dot(halfway, binormal), dot(halfway, normal));
}

// the effect's vertex program: oT0 = the screen position of (pos + normal x c7.x), c7.x = -0.125, divided by the
// vertex's own w, into the echo texture (pixel coordinates there; a uv shift here)
void lensShift() {
    vec4 moved = pc.mvp * vec4(inPos + inNrm * -0.125, 1.0);
    vLensShift = (moved.xy - gl_Position.xy) / gl_Position.w * 0.5;
}

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    lensShift();
    vec4 wp = pc.model * vec4(inPos, 1.0);
    vec3 wn = normalize(mat3(pc.model) * inNrm);
    vNrm = wn;
    vUV01 = inUV01;
    vUV23 = inUV23;
    vColour = scene.toggles2.z != 0 ? inColour : vec4(1.0, 1.0, 1.0, inColour.a);   // the baked lighting off: white
    vDepth = gl_Position.w;
    vRefl = reflect(normalize(wp.xyz - scene.camTime.xyz), wn);
    vec3 forward = -vec3(scene.view[0][2], scene.view[1][2], scene.view[2][2]);
    vReflFar = reflect(forward, wn);
    vNdl = vec2(max(dot(wn, -normalize(scene.sunDir.xyz)), 0.0), max(dot(wn, -scene.light1Dir.xyz), 0.0));
    vShellNdl = max(dot(wn, kShellLightTowards), 0.0);
    vDraw = gl_InstanceIndex;
    vMat = draws.d[gl_InstanceIndex].material;
    vWorld = wp.xyz;
    vTint = pc.model[3][3] > 1.5 ? vec3(pc.model[0][3], pc.model[1][3], pc.model[2][3]) : vec3(1.0);
    bumpVectors(wp.xyz);
}
