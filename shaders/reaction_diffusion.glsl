#version 460 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (rgba16f, binding = 0) uniform image3D grid;

uniform int width;
uniform int height;
uniform int depth;

uniform int depth_layer;
uniform int alternate;

uniform int space_step;
uniform int time_step;

uniform int a;
uniform int b;
uniform int D;

float U(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;

    if (alternate == 0)
        return imageLoad(grid, ivec3(x, y, z)).r;
    else
        return imageLoad(grid, ivec3(x, y, z)).b;
}

float V(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;
    if (alternate == 0)
        return imageLoad(grid, ivec3(x, y, z)).g;
    else
        return imageLoad(grid, ivec3(x, y, z)).a;
}

float dUdt(int x, int y, int z) {
    float u = U(x, y, z);
    float v = V(x, y, z);

    float d2Udx2 = (U(x+1, y, z) - 2 * u + U(x-1, y, z)) / pow(space_step, 2);
    float d2Udy2 = (U(x, y+1, z) - 2 * u + U(x, y-1, z)) / pow(space_step, 2);
    float d2Udz2 = (U(x, y, z+1) - 2 * u + U(x, y, z-1)) / pow(space_step, 2);

    return (d2Udx2 + d2Udy2 + d2Udz2) + (pow(u, 2) * v) - ((a + b) * u);
}

float dVdt(int x, int y, int z) {
    float u = U(x, y, z);
    float v = V(x, y, z);

    float d2Vdx2 = (V(x+1, y, z) - 2 * v + V(x-1, y, z)) / pow(space_step, 2);
    float d2Vdy2 = (V(x, y+1, z) - 2 * v + V(x, y-1, z)) / pow(space_step, 2);
    float d2Vdz2 = (V(x, y, z+1) - 2 * v + V(x, y, z-1)) / pow(space_step, 2);

    return D * (d2Vdx2 + d2Vdy2 + d2Vdz2) - (pow(u, 2) * v) + (a * (1 - v));
}

void main() {
    // ivec3 location = ivec3(gl_GlobalInvocationID.xy, depth_layer);
    ivec3 location = ivec3(gl_GlobalInvocationID.xyz);
    int x = location.x;
    int y = location.y;
    int z = location.z;

    // float ui = U(x, y, z);
    // float vi = V(x, y, z);
    // float dUdt = dUdt(x, y, z);
    // float dVdt = dVdt(x, y, z);
    // float uf = ui + dUdt * time_step;
    // float vf = vi + dVdt * time_step;

    // if (alternate == 0)
    //     imageStore(grid, location, vec4(ui, vi, uf, vf));
    // else
    //     imageStore(grid, location, vec4(uf, vf, ui, vi));
    
    imageStore(grid, ivec3(gl_GlobalInvocationID.xyz), vec4(x / float(width), y / float(height), z / float(depth), 1.0));
}