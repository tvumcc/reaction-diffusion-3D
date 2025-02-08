#version 460 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (rgba32f, binding = 0) uniform image3D grid;

uniform int width;
uniform int height;
uniform int depth;

uniform int depth_layer;
uniform int alternate;

uniform float space_step;
uniform float time_step;

uniform float F;
uniform float k;
uniform float Du;
uniform float Dv;

uniform bool paused;

float U(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;
    // if (pow(x-50, 2) + pow(y-50, 2) + pow(z-50, 2) > 20*20)
    //     return 0.0;

    return imageLoad(grid, ivec3(x, y, z)).r;
}

float V(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;
    // if (pow(x-50, 2) + pow(y-50, 2) + pow(z-50, 2) > 20*20)
    //     return 0.0;

    return imageLoad(grid, ivec3(x, y, z)).g;
}

float dUdt(int x, int y, int z) {
    float laplacian = U(x+1, y, z) + U(x-1, y, z) + U(x, y+1, z) + U(x, y-1, z) + U(x, y, z+1) + U(x, y, z-1) - 6.0 * U(x, y, z) / (space_step * space_step);
    // float laplacian = U(x, y+1, z) + U(x, y-1, z) + U(x, y, z+1) + U(x, y, z-1) - 4.0 * U(x, y, z);

    return Du * laplacian - (U(x, y, z) * pow(V(x, y, z), 2.0)) + F * (1.0 - U(x, y, z));
}

float dVdt(int x, int y, int z) {
    float laplacian = V(x+1, y, z) + V(x-1, y, z) + V(x, y+1, z) + V(x, y-1, z) + V(x, y, z+1) + V(x, y, z-1) - 6.0 * V(x, y, z) / (space_step * space_step);
    // float laplacian = V(x, y+1, z) + V(x, y-1, z) + V(x, y, z+1) + V(x, y, z-1) - 4.0 * V(x, y, z);

    return Dv * laplacian + (U(x, y, z) * pow(V(x, y, z), 2.0)) - (F + k) * V(x, y, z);
}

void main() {
    ivec3 location = ivec3(gl_GlobalInvocationID.xyz);
    int x = location.x;
    int y = location.y;
    int z = location.z;

    float dUdt = dUdt(x, y, z);
    float dVdt = dVdt(x, y, z);
    if (paused) {
        dUdt = 0.0;
        dVdt = 0.0;
    }
    float uf = U(x, y, z) + dUdt * time_step;
    float vf = V(x, y, z) + dVdt * time_step;

    // float ui = U(x, y, z);
    // float delta = 0.4 * dudt(x, y, z);
    // float uf = ui + dudt(x, y, z) * 0.1;

    imageStore(grid, location, vec4(uf, vf, 0.0, 1.0));
}