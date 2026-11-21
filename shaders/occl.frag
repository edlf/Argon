#version 450
// flare occlusion test: colour writes are masked off; only the samples that pass the depth test are counted
layout(location = 0) out vec4 outColour;
void main() { outColour = vec4(0.0); }
