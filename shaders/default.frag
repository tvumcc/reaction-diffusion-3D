#version 460 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D tex;
uniform float brightness;

void main() {
    FragColor = vec4(brightness * texture(tex, TexCoord).rgb, 1.0);
}
