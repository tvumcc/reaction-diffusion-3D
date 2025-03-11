#version 460 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (rgba32f, binding = 0) uniform image3D grid;

uniform float space_step;
uniform float time_step;

uniform float F;
uniform float k;
uniform float Du;
uniform float Dv;

uniform bool brush_enabled;
uniform int brush_x;
uniform int brush_y;
uniform int brush_z;

uniform bool paused;

void main() {
    ivec3 location = ivec3(gl_GlobalInvocationID.xyz);
    int x = location.x;
    int y = location.y;
    int z = location.z;

    vec4 grid_value = imageLoad(grid, location);

    float boundary_condition = grid_value.b;
    float u = grid_value.r * (-boundary_condition + 1.0);
    float v = grid_value.g * (-boundary_condition + 1.0);

    float dUdt = 0.0;
    float dVdt = 0.0;

    if (!paused) {
        vec4 neighbors[6];
        neighbors[0] = imageLoad(grid, ivec3(x+1, y, z));
        neighbors[1] = imageLoad(grid, ivec3(x-1, y, z));
        neighbors[2] = imageLoad(grid, ivec3(x, y+1, z));
        neighbors[3] = imageLoad(grid, ivec3(x, y-1, z));
        neighbors[4] = imageLoad(grid, ivec3(x, y, z+1));
        neighbors[5] = imageLoad(grid, ivec3(x, y, z-1));

        for (int i = 0; i < 6; i++)
            neighbors[i] *= vec4(vec2(-neighbors[i].b + 1.0), 1.0, 0.0);

        float laplacianU = neighbors[0].r + neighbors[1].r + neighbors[2].r + neighbors[3].r + neighbors[4].r + neighbors[5].r - 6.0 * u / (space_step * space_step);
        dUdt = Du * laplacianU - (u * pow(v, 2.0)) + F * (1.0 - u);

        float laplacianV = neighbors[0].g + neighbors[1].g + neighbors[2].g + neighbors[3].g + neighbors[4].g + neighbors[5].g - 6.0 * v / (space_step * space_step);
        dVdt = Dv * laplacianV + (u * pow(v, 2.0)) - (F + k) * v;
    }


    float uf = u + dUdt * time_step;
    float vf = v + dVdt * time_step;

    if (brush_enabled && (length(vec3(x - brush_x, y - brush_y, z - brush_z)) <= 1)) {
        imageStore(grid, location, vec4(1.0, 1.0, boundary_condition, 0.0));
    } else {
        imageStore(grid, location, vec4(uf, vf, boundary_condition, 0.0));
    }
}