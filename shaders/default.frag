#version 460 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

flat in ivec3 grid_pos;

uniform sampler3D grid_tex;

uniform vec3 cam_pos;

uniform float width;
uniform float height;
uniform float depth;

void main() {
    vec3 objectColor = vec3(1.0, 0.0, 0.0);
    vec3 lightDir = vec3(0.0, 0.0, -1.0);
    vec3 lightPos = vec3(1.0, 1.0, 1.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // vec3 cam_lighting = lightColor * max(0.0, dot(normalize(cam_pos - FragPos), normalize(Normal)));
    vec3 ambient = 0.4 * lightColor;
    // vec3 diffuse = max(dot(normalize(Normal), normalize(lightPos - FragPos)), 0.0) * lightColor;
    // vec3 specular = 0.5 * pow(max(dot(normalize(cam_pos - FragPos), reflect(-normalize(lightPos - FragPos), normalize(Normal))), 0.0), 32) * lightColor;

    // float brightness = float(imageLoad(grid, grid_pos).r);
    vec3 brightness = texture(grid_tex, vec3(float(grid_pos.x) / width, float(grid_pos.y) / height, float(grid_pos.z) / depth)).rgb;

    if (brightness.r < 0.3) discard;

    // FragColor = vec4((ambient + diffuse + specular) * objectColor, 0.4);
    FragColor = vec4(brightness, 1.0);
    // FragColor = vec4((cam_lighting) * objectColor, 0.4);
}
