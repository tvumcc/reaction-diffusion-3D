#version 460 core
out vec4 FragColor;

uniform vec3 object_color;
uniform float opacity;

void main() {
    FragColor = vec4(object_color, opacity);
}