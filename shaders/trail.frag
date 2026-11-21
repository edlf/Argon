#version 450
// newtrail.dds on stages 0 and 1 x vertex colour, drawn additive (ONE / ONE). The game's trail draw (0x9dca0)
// never sets the stage combine: it inherits whatever the last translucent mesh of the frame left (0xbbf60 ->
// 0xba660), so the viewer offers the likely combines (push constant model[0].z) and a gain (model[0].w).
layout(location = 0) in vec2 vUV0;
layout(location = 1) in vec2 vUV1;
layout(location = 2) in vec4 vCol;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } pc;
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];
layout(location = 0) out vec4 outColour;
void main() {
    vec3 a = texture(tex[0], vec3(vUV0, 0.0)).rgb;
    vec3 b = texture(tex[0], vec3(vUV1, 0.0)).rgb;
    int mode = int(pc.model[0].z);
    vec3 c = a * vCol.rgb;                                   // 0: stage 0 only (MODULATE)
    if (mode == 1) c = clamp(c * b, 0.0, 1.0);               // 1: stage 1 MODULATE
    else if (mode == 2) c = clamp(c * b * 2.0, 0.0, 1.0);    // 2: stage 1 MODULATE2X
    else if (mode == 3) c = clamp(c * b * 4.0, 0.0, 1.0);    // 3: stage 1 MODULATE4X
    else if (mode == 4) c = clamp(c + b * vCol.rgb, 0.0, 1.0);   // 4: stage 1 ADD
    outColour = vec4(c * pc.model[0].w, 1.0);
}
