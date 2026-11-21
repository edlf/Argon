// the order-independent translucency's composite (mesh_oit.frag fills the targets): the weighted average colour over
// the scene, covering it by 1 - the revealage (blend SRC_ALPHA / ONE_MINUS_SRC_ALPHA). The targets are the scene set's
// bindings 2 and 3; a pixel averages its samples (the scene's MSAA) so no per-sample shading is needed. Included by
// oit_composite.frag (one sample) and oit_composite_ms.frag, which define OIT_TARGET, oitSampleCount and oitFetch.
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColour;
layout(set = 1, binding = 2) uniform OIT_TARGET accumulatedTarget;
layout(set = 1, binding = 3) uniform OIT_TARGET revealageTarget;

void main() {
    const ivec2 pixel = ivec2(gl_FragCoord.xy);
    const int samples = oitSampleCount(accumulatedTarget);
    vec4 accumulated = vec4(0.0);
    float revealage = 0.0;
    for (int s = 0; s < samples; ++s) {
        accumulated += oitFetch(accumulatedTarget, pixel, s);
        revealage += oitFetch(revealageTarget, pixel, s).r;
    }
    accumulated /= float(samples);
    revealage /= float(samples);
    if (revealage >= 1.0) discard;   // nothing translucent here
    outColour = vec4(accumulated.rgb / max(accumulated.a, 1e-5), 1.0 - revealage);
}
