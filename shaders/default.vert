#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;

out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    // vec4 offset = vec4(0.5, 0.5, 0.5, 0.0);
    normal = aNormal;
    FragPos = vec3(model * (vec4(aPos, 1.0)));
    gl_PointSize = 10;
    gl_Position = proj * view * model * (vec4(aPos, 1.0));
}
