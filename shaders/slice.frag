#version 460 core

out vec4 FragColor;

in vec2 uv;

uniform sampler3D grid_tex;
uniform int slice_depth;
uniform int grid_resolution;

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
    vec4 brightness = texture(grid_tex, vec3(uv.x, 1.0 - uv.y, float(slice_depth) / float(grid_resolution)));
    vec3 color = viridis(2.0 * brightness.g);
    if (brightness.b >= 1.0) {
        color = vec3(1.0);
    }
    FragColor = vec4(color, 1.0);
}