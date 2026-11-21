#version 450
// Argon Options' order-independent translucency (render\order_independent_translucency.hpp): an alpha-blended
// translucent mesh shaded as mesh.frag does (mesh_shading.glsl), then added to the weighted blended targets
// (McGuire and Bavoil, "Weighted Blended Order-Independent Transparency", JCGT 2013) instead of blended in the game's
// draw order. Target 0 sums colour x alpha and alpha, both weighted (blend ONE / ONE); target 1 keeps the product of
// (1 - alpha) (blend ZERO / ONE_MINUS_SRC_COLOR). oit_composite.frag lays the result over the scene.
layout(location = 0) out vec4 outAccumulated;
layout(location = 1) out vec4 outRevealage;

vec4 outColour;

#include "mesh_shading.glsl"
#include "oit_split.glsl"

// the paper's depth weight (its equation 10) over the view distance in metres: nearer surfaces count for more
float depthWeight(float alpha) {
    return alpha * clamp(0.03 / (1e-5 + pow(vDepth / 200.0, 4.0)), 1e-2, 3e3);
}

void main() {
    shadeMesh();
    if (outColour.a >= kOitOpaqueAlpha) discard;   // drawn solid before the pass (mesh_oit_opaque.frag)
    float alpha = clamp(outColour.a, 0.0, 1.0);
    float weight = depthWeight(alpha);
    outAccumulated = vec4(outColour.rgb * alpha, alpha) * weight;
    outRevealage = vec4(alpha);
}
