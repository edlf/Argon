#version 450
// the models' colour straight into the scene (the shading: mesh_shading.glsl)
layout(location = 0) out vec4 outColour;

#include "mesh_shading.glsl"

void main() {
    shadeMesh();
}
