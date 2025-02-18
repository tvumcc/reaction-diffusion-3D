#version 460
layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

struct Vertex {
    vec3 pos;
    vec3 normal;
};

layout (binding = 1, std430) readonly buffer ssbo1 {int edge_table[256];};
layout (binding = 2, std430) readonly buffer ssbo2 {int vertex_table[24];};
layout (binding = 3, std430) readonly buffer ssbo3 {int triangle_table[4096];};
layout (binding = 4, std430) buffer ssbo4 {Vertex vertices[];};

uniform float width;
uniform float height;
uniform float depth;
uniform float threshold;
uniform sampler3D grid_tex;

void main() {
    vec3 pos = 2.0 * vec3(gl_GlobalInvocationID.x / width, gl_GlobalInvocationID.y / height, gl_GlobalInvocationID.z / depth);
    ivec3 loc = ivec3(gl_GlobalInvocationID.xyz);

    float shift = 2.0 / (width);

    vec3 shifts[8] = vec3[](
        vec3(0.0, 0.0, 0.0),
        vec3(0.0, 0.0, shift),
        vec3(shift, 0.0, shift),
        vec3(shift, 0.0, 0.0),
        vec3(0.0, shift, 0.0),
        vec3(0.0, shift, shift),
        vec3(shift, shift, shift),
        vec3(shift, shift, 0.0)
    );
    vec3 interpolated[12];
    vec3 normals[12];

    int cube_index = 0;
    for (int i = 0; i < 8; i++) {
        if (texture(grid_tex, pos + shifts[i]).g < threshold) {
            cube_index |= (1 << i);
        }
    }

    int edge_mask = edge_table[cube_index];
    int tri_index = cube_index * 16;

    // Interpolate edge vertices
    for (int i = 0; i < 12; i++) {
        if (((edge_mask >> i) & 1) == 1) {
            vec3 P1 = pos + shifts[vertex_table[i*2]];
            vec3 P2 = pos + shifts[vertex_table[i*2+1]];
            float V1 = texture(grid_tex, P1).g;
            float V2 = texture(grid_tex, P2).g;

            interpolated[i] = mix(P1, P2, (threshold - V1) / (V2 - V1));

            // Use gradients to compute the normal vectors
            vec3 P = interpolated[i]; 
            float delta = shift / 2.0;
            normals[i].x = texture(grid_tex, P + vec3(delta, 0.0, 0.0)).g - texture(grid_tex, P - vec3(delta, 0.0, 0.0)).g;
            normals[i].y = texture(grid_tex, P + vec3(0.0, delta, 0.0)).g - texture(grid_tex, P - vec3(0.0, delta, 0.0)).g;
            normals[i].z = texture(grid_tex, P + vec3(0.0, 0.0, delta)).g - texture(grid_tex, P - vec3(0.0, 0.0, delta)).g;
            normals[i] = -normalize(normals[i]);
        }
    }

    for (int i = 0; i < 15; i += 3) {
        int idx = (loc.x + loc.y * int(height / 2.0) + loc.z * int(width / 2.0) * int(height / 2.0)) * 15;
        if (triangle_table[tri_index + i] != -1) {
            vertices[idx + i + 0].pos    = interpolated[triangle_table[tri_index + i + 0]];
            vertices[idx + i + 0].normal = normals[triangle_table[tri_index + i + 0]];

            vertices[idx + i + 1].pos    = interpolated[triangle_table[tri_index + i + 1]];
            vertices[idx + i + 1].normal = normals[triangle_table[tri_index + i + 1]];

            vertices[idx + i + 2].pos    = interpolated[triangle_table[tri_index + i + 2]];
            vertices[idx + i + 2].normal = normals[triangle_table[tri_index + i + 2]];
        } else {
            vertices[idx + i + 0].pos = vec3(0.0);
            vertices[idx + i + 0].normal = vec3(0.0);
            vertices[idx + i + 1].pos = vec3(0.0);
            vertices[idx + i + 1].normal = vec3(0.0);
            vertices[idx + i + 2].pos = vec3(0.0);
            vertices[idx + i + 2].normal = vec3(0.0);
        }
    }
}