#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 normal;

uniform mat4 model;
uniform mat4 view_proj;

void main() {
    normal = aNormal;
    gl_Position = view_proj * model * (vec4(aPos, 1.0));
}
