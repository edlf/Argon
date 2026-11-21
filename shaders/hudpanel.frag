#version 450
// The game's HUD panel overlay (0xc1b40 + 0xc16f0): data\hud\feplasma0 / 1 / 2 / 0 on stages 0..3, uv per stage
// from the screen position (640 x 480 units) x 0.005 and t = (vblank count % 1000) / 1000 (60 vblanks a second):
//   stage 0 (x, y + t)   stage 1 (x + t, y)   stage 2 (-t, y)   stage 3 (x + t, y + t)
// colour ops MOD2X, MODULATE, MODULATE, MOD2X over the vertex colour, alpha MODULATE x4.
// The ImGui vertex carries uv = (x * 0.005 x scale + 8 + 32 * N, y * 0.005 x scale), N = vblanks % 1000 (both linear across
// the quad; the + 8 keeps panels that start left of the screen positive).
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2DArray sTex;
layout(location = 0) out vec4 fColor;
void main() {
    float n = floor(vUV.x / 32.0);
    float u = vUV.x - n * 32.0 - 8.0, v = vUV.y, t = n * 0.001;
    vec4 s0 = texture(sTex, vec3(u, v + t, 0.0));
    vec4 s1 = texture(sTex, vec3(u + t, v, 1.0));
    vec4 s2 = texture(sTex, vec3(-t, v, 2.0));
    vec4 s3 = texture(sTex, vec3(u + t, v + t, 0.0));
    vec3 c = clamp(s0.rgb * vColor.rgb * 2.0, 0.0, 1.0);
    c = c * s1.rgb;
    c = c * s2.rgb;
    c = clamp(c * s3.rgb * 2.0, 0.0, 1.0);
    fColor = vec4(c, vColor.a * s0.a * s1.a * s2.a * s3.a);
}
