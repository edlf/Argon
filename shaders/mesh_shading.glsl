// Emulates the game's fixed-function texture-stage cascade (Xbox D3D COLOROP/ALPHAOP per stage),
// set up per mesh by mesh_apply_material_stages (XBE 0x000ba660). See docs/FORMAT.md.
// Included by mesh.frag (the colour straight into the scene), mesh_oit.frag (the order-independent translucency's
// targets) and mesh_oit_opaque.frag (its solid pixels): each declares outColour, which shadeMesh writes, and calls
// shadeMesh from its main.
layout(location = 0) in vec3 vNrm;
layout(location = 1) in vec4 vUV01;
layout(location = 2) in vec4 vUV23;
layout(location = 3) in vec4 vColour;
layout(location = 4) in float vDepth;
layout(location = 5) in vec3 vRefl;
layout(location = 13) in vec3 vReflFar;   // the fixed-function texgen's reflection (non-local viewer, mesh.vert)
layout(location = 6) flat in int vMat;
layout(location = 9) in vec2 vNdl;        // N.L of light 0 / light 1 at the vertex (fixed-function lighting is per vertex)
layout(location = 7) in vec3 vWorld;
layout(location = 8) flat in vec3 vTint;
layout(location = 10) in vec3 vBumpLight;
layout(location = 11) in vec3 vBumpHalf;
layout(location = 12) in vec2 vLensShift;
layout(location = 14) in float vShellNdl;  // N.L of the shell light (mesh.vert, Spheremap Decal)
layout(location = 15) flat in int vDraw;   // the draw's record (mesh_draws.glsl)

layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
layout(set = 1, binding = 0) uniform Scene {
    vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    mat4 view;
    vec4 camTime;      // xyz camera, w time (s)
    ivec4 toggles;     // x fog, y lighting, z game materials, w fallback alpha mode (0 off, 1 auto, 2 all)
    ivec4 toggles2;    // x textures
    vec4 light1Colour, light1Dir;   // D3D light 1: a story scene's own light (black otherwise)
} scene;
layout(set = 1, binding = 1) uniform sampler2DArray frameCopy;   // the last frame (App::bindFrameCopy)
#include "mesh_draws.glsl"

struct Stage { ivec4 op; vec4 anim; vec4 extra; };
// hdr: x stage count, y has vertex colour, z kind (0 normal, 1 sky, 2 additive, 3..8 effects), w stage-0 alpha bits
// fog: the mesh's fog colour rgb and curve (-1 the track's, 0 off, 1 exp, 3 linear); fogShape: its density, range
struct Material { ivec4 hdr; vec4 params; vec4 fog; vec4 fogShape; Stage s[4]; };
layout(std430, set = 2, binding = 0) readonly buffer Materials { Material m[]; } mats;

// D3DTOP (Xbox numbering)
const int DISABLE = 1, SELECTARG1 = 2, SELECTARG2 = 3, MODULATE = 4, MODULATE2X = 5, MODULATE4X = 6,
          ADD = 7, ADDSIGNED = 8, ADDSIGNED2X = 9, SUBTRACT = 10, ADDSMOOTH = 11, BLENDDIFFUSEALPHA = 12,
          BLENDCURRENTALPHA = 13, BLENDTEXTUREALPHA = 14, BLENDFACTORALPHA = 15, BLENDTEXTUREALPHAPM = 16,
          PREMODULATE = 17, MODALPHA_ADDCOLOR = 18, MODCOLOR_ADDALPHA = 19, MODINVALPHA_ADDCOLOR = 20,
          MODINVCOLOR_ADDALPHA = 21, DOTPRODUCT3 = 22;

vec3 colourOp(int op, vec4 a1, vec4 a2, vec4 cur, vec4 t, vec4 dif) {
    switch (op) {
    case SELECTARG1: return a1.rgb;
    case SELECTARG2: return a2.rgb;
    case MODULATE2X: return a1.rgb * a2.rgb * 2.0;
    case MODULATE4X: return a1.rgb * a2.rgb * 4.0;
    case ADD: return a1.rgb + a2.rgb;
    case ADDSIGNED: return a1.rgb + a2.rgb - 0.5;
    case ADDSIGNED2X: return (a1.rgb + a2.rgb - 0.5) * 2.0;
    case SUBTRACT: return a1.rgb - a2.rgb;
    case ADDSMOOTH: return a1.rgb + a2.rgb - a1.rgb * a2.rgb;
    case BLENDDIFFUSEALPHA: return mix(a2.rgb, a1.rgb, dif.a);
    case BLENDCURRENTALPHA: return mix(a2.rgb, a1.rgb, cur.a);
    case BLENDTEXTUREALPHA: return mix(a2.rgb, a1.rgb, t.a);
    case BLENDFACTORALPHA: return a1.rgb;                      // texture factor not decoded (assume 1)
    case BLENDTEXTUREALPHAPM: return a1.rgb + a2.rgb * (1.0 - t.a);
    case PREMODULATE: return cur.rgb;                          // passes through; next texture is premodulated
    case MODALPHA_ADDCOLOR: return a1.rgb + a1.a * a2.rgb;
    case MODCOLOR_ADDALPHA: return a1.rgb * a2.rgb + a1.a;
    case MODINVALPHA_ADDCOLOR: return (1.0 - a1.a) * a2.rgb + a1.rgb;
    case MODINVCOLOR_ADDALPHA: return (1.0 - a1.rgb) * a2.rgb + a1.a;
    case DOTPRODUCT3: return vec3(clamp(4.0 * dot(a1.rgb - 0.5, a2.rgb - 0.5), 0.0, 1.0));
    }
    return a1.rgb * a2.rgb;   // MODULATE
}

float alphaOp(int op, vec4 a1, vec4 a2, vec4 cur, vec4 t, vec4 dif) {
    switch (op) {
    case SELECTARG1: return a1.a;
    case SELECTARG2: return a2.a;
    case MODULATE2X: return a1.a * a2.a * 2.0;
    case MODULATE4X: return a1.a * a2.a * 4.0;
    case ADD: return a1.a + a2.a;
    case ADDSIGNED: return a1.a + a2.a - 0.5;
    case ADDSIGNED2X: return (a1.a + a2.a - 0.5) * 2.0;
    case SUBTRACT: return a1.a - a2.a;
    case ADDSMOOTH: return a1.a + a2.a - a1.a * a2.a;
    case BLENDDIFFUSEALPHA: return mix(a2.a, a1.a, dif.a);
    case BLENDCURRENTALPHA: return mix(a2.a, a1.a, cur.a);
    case BLENDTEXTUREALPHA: return mix(a2.a, a1.a, t.a);
    case BLENDFACTORALPHA: return a1.a;
    case BLENDTEXTUREALPHAPM: return a1.a + a2.a * (1.0 - t.a);
    case PREMODULATE: return cur.a;
    case DOTPRODUCT3: return clamp(4.0 * dot(a1.rgb - 0.5, a2.rgb - 0.5), 0.0, 1.0);
    }
    return a1.a * a2.a;       // MODULATE
}

vec4 sampleStage(int i, vec3 uvw) {
    if (i == 0) return texture(tex[0], uvw);
    if (i == 1) return texture(tex[1], uvw);
    if (i == 2) return texture(tex[2], uvw);
    return texture(tex[3], uvw);
}

// cube map stored as a 6-layer array (+X -X +Y -Y +Z -Z) in stage slot 1
vec3 cubeArray(vec3 r) {
    vec3 a = abs(r);
    float face, ma, sc, tc;
    if (a.x >= a.y && a.x >= a.z) { face = r.x > 0.0 ? 0.0 : 1.0; ma = a.x; sc = r.x > 0.0 ? -r.z : r.z; tc = -r.y; }
    else if (a.y >= a.z)          { face = r.y > 0.0 ? 2.0 : 3.0; ma = a.y; sc = r.x; tc = r.y > 0.0 ? r.z : -r.z; }
    else                          { face = r.z > 0.0 ? 4.0 : 5.0; ma = a.z; sc = r.z > 0.0 ? r.x : -r.x; tc = -r.y; }
    return textureLod(tex[1], vec3((vec2(sc, tc) / max(ma, 1e-6) + 1.0) * 0.5, face), 0.0).rgb;
}

// the fog setter 0xa8450's curves: 0 off, 1 and 2 EXP, 3 LINEAR (render\mesh_fog.hpp)
float fogFactor(float curve, float density, float range) {
    if (curve < 0.5) return 1.0;
    if (curve < 2.5) return exp(-density * vDepth);
    return clamp(1.0 - vDepth / max(range, 1.0), 0.0, 1.0);
}

vec3 trackFog(vec3 rgb) {
    return mix(scene.fogColour.rgb, rgb, fogFactor(scene.fogParams.x, scene.fogParams.y, scene.fogParams.z));
}

// the final combiner (read live, tools\xemu\combiners.py): fog.a x r0 + (1 - fog.a) x the fog colour, for every mesh
// the surface setup leaves fog on, additive ones too (the fog colour is then added to the frame)
vec3 meshFog(Material M, vec3 rgb) {
    if (M.fog.w < -0.5) return trackFog(rgb);
    return mix(M.fog.rgb, rgb, fogFactor(M.fog.w, M.fogShape.x, M.fogShape.y));
}

vec3 sceneAmbient() { vec4 own = draws.d[vDraw].ambient; return own.w > 0.5 ? own.rgb : scene.ambient.rgb; }
vec3 sceneLight0() { vec4 own = draws.d[vDraw].light0; return own.w > 0.5 ? own.rgb : scene.sunColour.rgb; }

// the game's fixed-function lighting (src/render/d3d_lighting.hpp): one material (0xa6ad7: ambient 1, emissive 0),
// diffuse from the vertex colour, ambient from the material, no specular: clamp(ambient + vertex colour x (light 0 x
// N.L0 + light 1 x N.L1)). Material bits 64 / 128: the ambient is the surface's own colour / black, 256 / 512: light 0
// is. The own colour is the lighting block's (params.yzw) x the bone tint x intensity (vTint). Light 1 is a story
// scene's (0x9a810); skinned meshes (bit 8, shaders/char*bone.xvu from 0xa3df0) clamp before adding it:
// min(min(v7 x N.c5 x c9 + c8, 1) + v7 x N.c36 x c37, 1), c36 / c37 its direction (negated) and colour.
vec3 d3dLighting(Material M) {
    vec3 own = M.params.yzw * vTint;
    int bits = M.hdr.w;
    vec3 ambient = (bits & 64) != 0 ? own : (bits & 128) != 0 ? vec3(0.0) : sceneAmbient();
    vec3 light = (bits & 256) != 0 ? own : (bits & 512) != 0 ? vec3(0.0) : sceneLight0();
    vec3 fromLight1 = vColour.rgb * scene.light1Colour.rgb * vNdl.y;
    vec3 fromLight0 = ambient + vColour.rgb * light * vNdl.x;
    if ((bits & 8) != 0) {
        return clamp(clamp(fromLight0, 0.0, 1.0) + fromLight1, 0.0, 1.0);
    }
    return clamp(fromLight0 + fromLight1, 0.0, 1.0);
}

// "Spheremap Decal" (kind 8, program from 0x7a920): oD0 = min(min(v3 x max(N.c5, 0) x c9 + c8, 1) + v3 x max(N.c36, 0)
// x c37, 1). c8 / c9 are the shell light's ambient 0x40 and colour 0xcc (0x3910f4 / 0x391100 / 255, set by 0xaf360 and
// the init 0x76d60), not D3D's ambient 0x707070; 0xaf3d0 makes both black for a locked character, as it turns light 0
// off (the viewer's front end then has a black light 0). c36 / c37: light 1, a story scene's (black otherwise).
vec3 spheremapDecalLighting() {
    bool shellLit = any(greaterThan(sceneLight0(), vec3(0.0)));
    float ambient = shellLit ? 64.0 / 255.0 : 0.0, light = shellLit ? 204.0 / 255.0 : 0.0;
    vec3 fromShell = min(vColour.rgb * vShellNdl * light + ambient, vec3(1.0));
    return min(fromShell + vColour.rgb * scene.light1Colour.rgb * vNdl.y, vec3(1.0));
}

// an animated texture's frame (d3d_set_texture 0xcbb40): round(clock x rate) mod the frame count, the rate the
// stage's own or, when the bind was skipped (0xcbbd2), that of the draw that bound the texture
float animationFrame(Stage st, int stage) {
    float boundRate = draws.d[vDraw].stageRates[stage];
    float rate = boundRate >= 0.0 ? boundRate : st.anim.x;
    return st.anim.y > 1.0 ? mod(floor(scene.camTime.w * rate + 0.5), st.anim.y) : 0.0;
}

vec4 stageTexture(Stage st, int stage, vec2 uv) {
    return sampleStage(stage, vec3(uv, animationFrame(st, stage)));
}

// "Diffuse Specular Bump Map" (kind 5): the pixel shader handle the draw binds (0x2907f8, loaded by 0xa2014) holds
// shaders/diffusespecalpha.xpu (tools/cmm/xpu.py; read back from xemu's memory), constants from 0x7d6e0: c0 ambient,
// c1 light 0's colour (the sun), c2 min(2 x sun, 1). t0 colour (UV set 0), t1 normal map (UV set 1), t2 / t3 the
// normalisation cube map at the light / half vectors.
//   r1 = clamp(N.L) x v0 + c0;  r0.rgb = r1 x t0;      the ambient is textured too
//   r1 = clamp(N.H)^8 x t0.a;                          the colour texture's alpha masks the highlight
//   out = r0.rgb x c1 + r1 x c2
// No fog: 0x7d73b turns it off (0xa8450 mode 0) and the final combiner has no fog term. The program's alpha (c0.a = 0)
// never shows: every such surface is opaque without the alpha test, and the back buffer is R5G6B5 (0xa681a), so
// a read-back of the frame sees alpha 1.
vec3 diffuseSpecularBump(Material M) {
    vec4 colour = scene.toggles2.x != 0 ? stageTexture(M.s[0], 0, vUV01.xy) : vec4(1.0);
    vec4 bump = scene.toggles2.x != 0 ? stageTexture(M.s[1], 1, vUV01.zw) : vec4(0.5, 0.5, 1.0, 1.0);
    vec3 normal = bump.rgb * 2.0 - 1.0;
    float diffuse = clamp(dot(normal, normalize(vBumpLight)), 0.0, 1.0);
    float specular = pow(clamp(dot(normal, normalize(vBumpHalf)), 0.0, 1.0), 8.0) * colour.a;
    vec3 ambient = M.params.y >= 0.0 ? M.params.yzw : sceneAmbient();
    vec3 sun = sceneLight0();
    vec3 lit = clamp(clamp(diffuse * vColour.rgb + ambient, 0.0, 1.0) * colour.rgb, 0.0, 1.0);
    return clamp(lit * sun + specular * min(2.0 * sun, vec3(1.0)), 0.0, 1.0);
}

void shadeMesh() {
    Material M = mats.m[vMat];
    float time = scene.camTime.w;
    bool gameMat = scene.toggles.z != 0;
    bool lit = scene.toggles.y != 0;
    int kind = M.hdr.z;
    bool sky = kind == 1, additive = kind == 2;

    // diffuse: prelit vertex colour (vtype 0..3), otherwise D3D lighting stand-in (sun + ambient)
    vec4 dif = vColour;
    if (gameMat && kind == 8) {
        dif.rgb = spheremapDecalLighting();
    } else if (M.hdr.y != 0 && scene.toggles2.y != 0 && !sky && !additive && (M.hdr.w & 32) != 0) {
        // front end: the surface setup's lighting whatever the material toggle (FE TITLE's logo faces, setup 2 with
        // flag 0x20, are unlit: their white vertex colour x the gradient texture)
        dif.rgb = d3dLighting(M);
    } else if (gameMat && lit && (M.hdr.w & 32) != 0 && !sky) {
        // a lit surface in the race: the mega-up's cage gets its blues this way, the sun's lens flare its colour
        // (setup 1 with 0x8000 + 0x2000 + 4: the own colour x the occlusion intensity is all the light)
        dif.rgb = d3dLighting(M);
    } else if (M.hdr.y == 0) {
        dif = vec4(1.0);
        if (lit && !sky && !additive) {
            dif.rgb = clamp(scene.ambient.rgb + scene.sunColour.rgb * vNdl.x, 0.0, 1.0);
        }
    }

    if (gameMat && (kind == 3 || kind == 4)) {
        // "Specular Bump" (3) / "Specular Bump RGBA" (4): texm3x3vspec with the stage-0 normal map; tangent
        // frame from the screen-space derivatives of position and uv (the game builds it per vertex, 0xa2a00)
        Stage st = M.s[0];
        vec2 uv = st.op.w == 0 ? vUV01.xy : st.op.w == 1 ? vUV01.zw : st.op.w == 2 ? vUV23.xy : vUV23.zw;
        uv += st.anim.zw * time + st.extra.xy;
        vec4 b = scene.toggles2.x != 0 ? sampleStage(0, vec3(uv, animationFrame(st, 0))) : vec4(0.5, 0.5, 1.0, 1.0);
        vec3 bn = b.rgb * 2.0 - 1.0;
        vec3 N = normalize(vNrm);
        vec3 dp1 = dFdx(vWorld), dp2 = dFdy(vWorld);
        vec2 du1 = dFdx(uv), du2 = dFdy(uv);
        float det = du1.x * du2.y - du2.x * du1.y;
        vec3 T = (dp1 * du2.y - dp2 * du1.y) * sign(det), B = (dp2 * du1.x - dp1 * du2.x) * sign(det);
        T = T - N * dot(N, T); B = B - N * dot(N, B);
        T = dot(T, T) > 1e-12 ? normalize(T) : vec3(1, 0, 0);
        B = dot(B, B) > 1e-12 ? normalize(B) : vec3(0, 0, 1);
        vec3 Nw = T * bn.x + B * bn.y + N * bn.z;
        vec3 E = scene.camTime.xyz - vWorld;
        vec3 R = 2.0 * Nw * dot(Nw, E) / max(dot(Nw, Nw), 1e-6) - E;
        vec3 refl = cubeArray(R);
        vec4 res = kind == 3 ? vec4(refl, b.a) : vec4(refl * vColour.rgb, vColour.a);
        if (res.a < M.params.x) discard;
        outColour = vec4(scene.toggles.x != 0 ? trackFog(res.rgb) : res.rgb, res.a);
        return;
    }

    if (gameMat && kind == 6) {
        // "Hud Lens" (0x7a6a0): no pixel shader; stage 0 = the echo texture 0x50e160 at the vertex program's screen
        // position, modulated by oD0 = vertex colour x 255 (white wherever it is not black); blending off
        vec2 uv = gl_FragCoord.xy / vec2(textureSize(frameCopy, 0).xy) + vLensShift;
        vec3 behind = texture(frameCopy, vec3(uv, 0.0)).rgb;
        outColour = vec4(behind * clamp(vColour.rgb * 255.0, 0.0, 1.0), clamp(vColour.a * 255.0, 0.0, 1.0));
        return;
    }

    if (gameMat && kind == 5) {
        outColour = vec4(diffuseSpecularBump(M), 1.0);
        return;
    }

    if (gameMat && kind == 7) {
        // "Color Scale No Texture" (0x7a4b0): oD0 = vertex colour x c4, c4 = the bone's tint x occlusion intensity
        // (vTint); no textures, no lighting, the fog off (0xa8450 mode 0). Every such mesh blends ONE / ONE, so the
        // alpha (also scaled in the game) never shows
        outColour = vec4(clamp(vColour.rgb * vTint, 0.0, 1.0), vColour.a);
        return;
    }

    vec4 cur = dif;
    int n = gameMat ? M.hdr.x : min(M.hdr.x, 1);
    vec3 premod = vec3(1.0);
    for (int i = 0; i < 4; ++i) {
        if (i >= n) break;
        Stage st = M.s[i];
        int cop = gameMat ? st.op.x : MODULATE, aop = gameMat ? st.op.y : MODULATE;
        // light 0's colour black (a locked character on FE CHARSELECT, 0xaf3d0): mesh_apply_material_stages
        // (0xbad5b, light colour 0x50e2f8) sets every stage's colour op to code 0 = MODULATE unless the surface has
        // flag 0x2000, and the skinning setup (0xa3df0) does the same for skinned meshes; so the mesh takes the
        // (black) lit colour and only unlit glows remain (Kutoo's eye quads)
        if (gameMat && scene.toggles2.y != 0 && all(equal(sceneLight0(), vec3(0.0))) &&
            ((M.hdr.w & 8) != 0 || (M.hdr.w & 16) == 0))
            cop = MODULATE;
        if (cop == DISABLE) break;
        int flags = gameMat ? st.op.z : 0;
        vec2 uv = st.op.w == 0 ? vUV01.xy : st.op.w == 1 ? vUV01.zw : st.op.w == 2 ? vUV23.xy : vUV23.zw;
        if ((flags & 8) != 0) uv = (vReflFar.xy + 1.0) * vec2(0.5, -0.5);       // env map (texture matrix 0x50e120)
        // "Spheremap Decal" (kind 8, 0x7a920): the effect skips the env setup; its program's oT1 = 0.5 - 0.5 x the
        // world reflection's x and y (c11 = (1, 0, 0.5, -0.5)) whatever stage 1's flags say
        if (gameMat && kind == 8 && i == 1) uv = 0.5 - 0.5 * vRefl.xy;
        // its stage 2 (Jaya's head: ADD) reads oT2, which the program never writes: an unwritten output is
        // (0, 0, 0, 1) as xemu gives it, so the stage adds one texel. Stage 1's texture matrix does not apply: the
        // NV2A's texture transform is fixed-function T&L, skipped under a vertex program (COUNT2: no divide)
        if (gameMat && kind == 8 && i == 2) uv = vec2(0.0);
        if ((flags & 1) != 0) uv += st.anim.zw * time + st.extra.xy;             // uv scroll
        vec4 t = scene.toggles2.x != 0 ? sampleStage(i, vec3(uv, animationFrame(st, i))) : vec4(1.0);
        if (st.extra.z > 0.5 && scene.toggles2.x != 0) {
            // detail normal map: shade by a fixed light direction instead of multiplying the raw colour
            vec3 nm = normalize(t.rgb * 2.0 - 1.0);
            float f = clamp(dot(nm, normalize(vec3(0.35, 0.35, 0.87))) * 1.08, 0.0, 1.0);
            t = vec4(vec3(f), 1.0);
        }
        t.rgb *= premod;
        premod = vec3(1.0);
        bool swapArgs = (flags & 0x100) != 0;            // CURRENT op TEXTURE instead of TEXTURE op CURRENT
        vec4 a1 = swapArgs ? cur : t, a2 = swapArgs ? t : cur;
        vec4 r = vec4(colourOp(cop, a1, a2, cur, t, dif), alphaOp(aop, a1, a2, cur, t, dif));
        if (cop == PREMODULATE) premod = t.rgb;
        cur = clamp(r, 0.0, 1.0);
    }

    float ref = 0.0;
    if (gameMat) ref = M.params.x;
    else if (scene.toggles.w == 2 ? (M.hdr.w & 2) != 0 : scene.toggles.w == 1 ? (M.hdr.w & 1) != 0 : false) ref = 0.5;
    float aw = fwidth(cur.a);
    if (gameMat && (M.hdr.w & 4) != 0 && ref > 0.0) {
        // alpha-to-coverage pipeline: sharpen the test edge to about one pixel so MSAA can smooth it
        float a = (cur.a - ref) / max(aw, 1e-4) + 0.5;
        if (a <= 0.0) discard;
        cur.a = clamp(a, 0.0, 1.0);
    } else if (cur.a < ref) discard;

    vec3 rgb = cur.rgb;
    if (scene.toggles.x != 0 && !sky) {
        rgb = meshFog(M, rgb);
    }
    outColour = vec4(rgb, cur.a);
}
