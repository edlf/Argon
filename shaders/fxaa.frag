#version 450
// FXAA, fast approximate anti-aliasing (after Timothy Lottes' FXAA 3.11, its quality preset): a pixel on a luma edge
// finds the edge's direction, walks along it to both ends, and samples the frame half a pixel across the edge in
// proportion to how near the nearer end is; single pixels standing out of their neighbours are blended too.
layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];   // 0: the frame so far (the 3D view, before the HUD)
layout(push_constant) uniform Push { vec4 rtMetrics; } pc;    // 1 / width, 1 / height, width, height
layout(location = 0) out vec4 outColour;

const float kContrastThreshold = 0.125;       // the local contrast an edge needs, of its brightest luma
const float kContrastThresholdMin = 0.0312;   // and at least this (dark areas)
const float kSubpixelBlend = 0.75;            // how much a lone pixel is blended into its neighbours
const int kSearchSteps = 12;
const float kSearchStride[kSearchSteps] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);

vec3 frameAt(vec2 uv) {
    return textureLod(tex[0], vec3(uv, 0.0), 0.0).rgb;
}

// the frame's values are gamma encoded: the luma is taken on them as they are
float lumaOf(vec3 colour) {
    return dot(colour, vec3(0.299, 0.587, 0.114));
}

float lumaAt(vec2 uv) {
    return lumaOf(frameAt(uv));
}

float lumaNear(vec2 pixels) {
    return lumaAt(vUV + pixels * pc.rtMetrics.xy);
}

void main() {
    vec3 centre = frameAt(vUV);
    float lumaM = lumaOf(centre);
    float lumaN = lumaNear(vec2(0, -1)), lumaS = lumaNear(vec2(0, 1));
    float lumaW = lumaNear(vec2(-1, 0)), lumaE = lumaNear(vec2(1, 0));
    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float contrast = lumaMax - lumaMin;
    if (contrast < max(kContrastThresholdMin, lumaMax * kContrastThreshold)) {
        outColour = vec4(centre, 1.0);
        return;
    }
    float lumaNW = lumaNear(vec2(-1, -1)), lumaNE = lumaNear(vec2(1, -1));
    float lumaSW = lumaNear(vec2(-1, 1)), lumaSE = lumaNear(vec2(1, 1));

    // a horizontal edge changes most from row to row
    float rowChange = abs(lumaNW + lumaSW - 2.0 * lumaW) + 2.0 * abs(lumaN + lumaS - 2.0 * lumaM) + abs(lumaNE + lumaSE - 2.0 * lumaE);
    float columnChange = abs(lumaNW + lumaNE - 2.0 * lumaN) + 2.0 * abs(lumaW + lumaE - 2.0 * lumaM) + abs(lumaSW + lumaSE - 2.0 * lumaS);
    bool horizontal = rowChange >= columnChange;

    // the side across the edge with the larger change: N / W (towards -1) or S / E
    float lumaBefore = horizontal ? lumaN : lumaW, lumaAfter = horizontal ? lumaS : lumaE;
    bool towardsBefore = abs(lumaBefore - lumaM) >= abs(lumaAfter - lumaM);
    float gradientLimit = 0.25 * max(abs(lumaBefore - lumaM), abs(lumaAfter - lumaM));
    float across = (horizontal ? pc.rtMetrics.y : pc.rtMetrics.x) * (towardsBefore ? -1.0 : 1.0);
    float lumaEdge = 0.5 * (lumaM + (towardsBefore ? lumaBefore : lumaAfter));

    // walk both ways along the edge, half a pixel across it, until the luma leaves the edge's
    vec2 along = horizontal ? vec2(pc.rtMetrics.x, 0.0) : vec2(0.0, pc.rtMetrics.y);
    vec2 onEdge = vUV + (horizontal ? vec2(0.0, 0.5 * across) : vec2(0.5 * across, 0.0));
    vec2 uvBack = onEdge - along, uvAhead = onEdge + along;
    float endBack = lumaAt(uvBack) - lumaEdge, endAhead = lumaAt(uvAhead) - lumaEdge;
    bool backDone = abs(endBack) >= gradientLimit, aheadDone = abs(endAhead) >= gradientLimit;
    for (int step = 1; step < kSearchSteps && !(backDone && aheadDone); ++step) {
        if (!backDone) {
            uvBack -= along * kSearchStride[step];
            endBack = lumaAt(uvBack) - lumaEdge;
            backDone = abs(endBack) >= gradientLimit;
        }
        if (!aheadDone) {
            uvAhead += along * kSearchStride[step];
            endAhead = lumaAt(uvAhead) - lumaEdge;
            aheadDone = abs(endAhead) >= gradientLimit;
        }
    }
    float distanceBack = horizontal ? vUV.x - uvBack.x : vUV.y - uvBack.y;
    float distanceAhead = horizontal ? uvAhead.x - vUV.x : uvAhead.y - vUV.y;
    bool backNearer = distanceBack < distanceAhead;
    float edgeShift = 0.5 - min(distanceBack, distanceAhead) / (distanceBack + distanceAhead);
    // shifted only when the nearer end turns the way the centre does (else this pixel is outside the edge's step)
    bool shiftsHere = ((backNearer ? endBack : endAhead) < 0.0) != (lumaM < lumaEdge);

    float lumaAverage = (2.0 * (lumaN + lumaS + lumaW + lumaE) + lumaNW + lumaNE + lumaSW + lumaSE) / 12.0;
    float standsOut = clamp(abs(lumaAverage - lumaM) / contrast, 0.0, 1.0);
    float smoothStandsOut = (3.0 - 2.0 * standsOut) * standsOut * standsOut;
    float subpixelShift = smoothStandsOut * smoothStandsOut * kSubpixelBlend;

    float shift = max(shiftsHere ? edgeShift : 0.0, subpixelShift) * across;
    vec2 uv = vUV + (horizontal ? vec2(0.0, shift) : vec2(shift, 0.0));
    outColour = vec4(frameAt(uv), 1.0);
}
