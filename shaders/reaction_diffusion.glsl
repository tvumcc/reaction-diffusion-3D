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

uniform int a;
uniform int b;
uniform int D;

float U(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;

    return imageLoad(grid, ivec3(x, y, z)).r;
}

float V(int x, int y, int z) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        return 0.0;

    return imageLoad(grid, ivec3(x, y, z)).g;
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

float dudt(int x, int y, int z) {
    float u = U(x, y, z);

    float d2Udx2 = (U(x+1, y, z) - 2 * u + U(x-1, y, z)) / pow(space_step, 2);
    float d2Udy2 = (U(x, y+1, z) - 2 * u + U(x, y-1, z)) / pow(space_step, 2);
    float d2Udz2 = (U(x, y, z+1) - 2 * u + U(x, y, z-1)) / pow(space_step, 2);

    return (d2Udx2 + d2Udy2 + d2Udz2);
}

void main() {
    ivec3 location = ivec3(gl_GlobalInvocationID.xyz);
    int x = location.x;
    int y = location.y;
    int z = location.z;

    // float ui = U(x, y, z);
    // float vi = V(x, y, z);
    // float dUdt = dUdt(x, y, z);
    // float dVdt = dVdt(x, y, z);
    // float uf = ui + dUdt * 0.01;
    // float vf = vi + dVdt * 0.01;

    float ui = U(x, y, z);
    float delta = 0.4 * dudt(x, y, z);
    float uf = ui + dudt(x, y, z) * 0.1;

    imageStore(grid, location, vec4(uf, 0.0, 0.0, 1.0));
}