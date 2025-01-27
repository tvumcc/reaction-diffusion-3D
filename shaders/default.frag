#version 460 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 cam_pos;

void main() {
    vec3 objectColor = vec3(1.0, 0.0, 0.0);
    vec3 lightDir = vec3(0.0, 0.0, -1.0);
    vec3 lightPos = vec3(1.0, 1.0, 1.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // vec3 cam_lighting = lightColor * max(0.0, dot(normalize(cam_pos - FragPos), normalize(Normal)));
    vec3 ambient = 0.4 * lightColor;
    // vec3 diffuse = max(dot(normalize(Normal), normalize(lightPos - FragPos)), 0.0) * lightColor;
    // vec3 specular = 0.5 * pow(max(dot(normalize(cam_pos - FragPos), reflect(-normalize(lightPos - FragPos), normalize(Normal))), 0.0), 32) * lightColor;

    // FragColor = vec4((ambient + diffuse + specular) * objectColor, 0.4);
    FragColor = vec4((ambient) * objectColor, 0.4);
    // FragColor = vec4((cam_lighting) * objectColor, 0.4);
}
