#version 450
// one triangle over the whole target from gl_VertexIndex 0..2 (no vertex buffer); vUV runs 0..1 across the target,
// (0, 0) top left: the post-process anti-aliasing passes (render\post_anti_aliasing.hpp)
layout(location = 0) out vec2 vUV;

void main() {
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
