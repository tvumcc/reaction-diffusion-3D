#version 460 core
out vec4 FragColor;

in vec3 normal;

void main() {
    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
}
