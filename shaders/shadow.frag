#version 450
// texture x vertex colour, drawn with ONE / ONE REVERSE_SUBTRACT (0x9c3a0): framebuffer - silhouette * grey
layout(location = 0) in vec3 vUV;
layout(location = 1) in vec3 vCol;
layout(set = 0, binding = 0) uniform sampler2DArray tex[4];   // [0] = shadow maps, one layer per ship
layout(location = 0) out vec4 outColour;
void main() {
    vec3 s = texture(tex[0], vUV).rgb;
    outColour = vec4(s * vCol, 0.0);
}
