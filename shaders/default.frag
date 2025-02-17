#version 460 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 pos;
in vec3 normal;

flat in ivec3 grid_pos;
flat in float tester;

uniform sampler3D grid_tex;

uniform vec3 cam_pos;

uniform float width;
uniform float height;
uniform float depth;

uniform float threshold;

vec3 viridis(float t) {
    const vec3 c0 = vec3(0.274344,0.004462,0.331359);
    const vec3 c1 = vec3(0.108915,1.397291,1.388110);
    const vec3 c2 = vec3(-0.319631,0.243490,0.156419);
    const vec3 c3 = vec3(-4.629188,-5.882803,-19.646115);
    const vec3 c4 = vec3(6.181719,14.388598,57.442181);
    const vec3 c5 = vec3(4.876952,-13.955112,-66.125783);
    const vec3 c6 = vec3(-5.513165,4.709245,26.582180);
    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}

void main() {
    vec3 objectColor = vec3(1.0, 0.0, 0.0);
    vec3 lightDir = vec3(0.0, 0.0, -1.0);
    vec3 lightPos = vec3(1.0, 1.0, 1.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 cam_lighting = lightColor * max(0.0, dot(normalize(cam_pos - FragPos), normalize(normal)));
    vec3 ambient = 0.4 * lightColor;
    vec3 diffuse = max(dot(normalize(Normal), normalize(lightPos - FragPos)), 0.0) * lightColor;
    vec3 specular = 0.5 * pow(max(dot(normalize(cam_pos - FragPos), reflect(-normalize(lightPos - FragPos), normalize(Normal))), 0.0), 32) * lightColor;

    // float brightness = float(imageLoad(grid, grid_pos).r);
    // vec3 brightness = texture(grid_tex, vec3(float(grid_pos.x) / width, float(grid_pos.y) / height, float(grid_pos.z) / depth)).rgb;
    vec3 brightness = texture(grid_tex, pos).rgb;

    // if (brightness.b < threshold) discard;

    // FragColor = vec4(viridis(2.0 * brightness.g), 1.0);
    // FragColor = vec4(vec3(tester / 256.0, 0.0, 0.0), 1.0);
    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
    // FragColor = vec4(vec3(brightness.b), 1.0);
    // FragColor = vec4(objectColor * cam_lighting, 1.0);
    // FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}
