#version 460 core
layout (points) in;
// layout (triangle_strip, max_vertices = 3) out;
layout (points, max_vertices = 1) out;

layout (binding = 1, std430) readonly buffer ssbo1 {
    int[] edge_table;
};

layout (binding = 2, std430) readonly buffer ssbo2 {
    int[] vertex_table;
};

layout (binding = 3, std430) readonly buffer ssbo3 {
    int[] triangle_table;
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform float width;
uniform float height;
uniform float depth;

uniform float threshold;

uniform sampler3D grid_tex;
flat out ivec3 grid_pos;

out vec3 normal;

void main() {
    grid_pos = ivec3(gl_in[0].gl_Position.x * width, gl_in[0].gl_Position.y * height, gl_in[0].gl_Position.z * depth);
    vec3 offset = vec3(0.5, 0.5, 0.5);
    vec3 pos = gl_in[0].gl_Position.xyz;

    // if (grid_pos.x == int(width)-1 || grid_pos.y == int(height)-1 || grid_pos.z == int(depth)-1) {
    //     return;
    // }

    vec3 shifts[8] = vec3[](
        vec3(0.0, 0.0, 0.0),
        vec3(1.0 / width, 0.0, 0.0),
        vec3(1.0 / width, 1.0 / width, 0.0),
        vec3(0.0, 1.0 / width, 0.0),
        vec3(0.0, 0.0, 1.0 / width),
        vec3(1.0 / width, 0.0, 1.0 / width),
        vec3(1.0 / width, 1.0 / width, 1.0 / width),
        vec3(0.0, 1.0 / width, 1.0 / width)
    );

    // // vec3 normals[8] = vec3[](
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0),
    // //     vec3(0.0, 0.0, 0.0)
    // // );

    // vec3 interpolated[12] = vec3[](
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0)
    // );

    // vec3 interpolated_normals[12] = vec3[](
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0),
    //     vec3(0.0, 0.0, 0.0)
    // );

    // int cube_index = 0;
    // for (int i = 0; i < 8; i++) {
    //     if (texture(grid_tex, pos + shifts[i]).g < threshold) {
    //         cube_index |= (1 << i);
    //     }
    // }

    // int edge_mask = edge_table[cube_index];
    // int tri_index = cube_index * 16;

    // Generate vertex normals
    // for (int i = 0; i < 8; i++) {
    //     vec3 P = vec3(grid_pos) + shifts[i]; 
    //     float delta = 0.01;

    //     normals[i].x = texture(grid_tex, P + vec3(delta, 0.0, 0.0)).g - texture(grid_tex, P - vec3(delta, 0.0, 0.0)).g;
    //     normals[i].y = texture(grid_tex, P + vec3(0.0, delta, 0.0)).g - texture(grid_tex, P - vec3(0.0, delta, 0.0)).g;
    //     normals[i].z = texture(grid_tex, P + vec3(0.0, 0.0, delta)).g - texture(grid_tex, P - vec3(0.0, 0.0, delta)).g;
    //     normals[i] = normalize(normals[i]);
    // }

    // Interpolate edge vertices
    // for (int i = 0; i < 12; i++) {
    //     if (((edge_mask >> i) & 1) == 1) {
    //         vec3 P1 = pos + shifts[vertex_table[i*2]];
    //         vec3 P2 = pos + shifts[vertex_table[i*2+1]];
    //         float V1 = texture(grid_tex, P1).g;
    //         float V2 = texture(grid_tex, P2).g;

    //         interpolated[i] = P1 + (threshold - V1) * (P2 - P1) / (V2 - V1);
    //         // interpolated_normals[i] = mix(normals[vertex_table[i*2]], normals[vertex_table[i*2+1]], (threshold - V1));
    //     }
    // }

    // Generate triangles
    // for (int i = 0; i < 3; i += 3) {
    //     if (triangle_table[tri_index + i] == -1) break;

    //     gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i]], 0.0) - offset);
    //     // normal = interpolated_normals[tri_index + i];
    //     EmitVertex();

    //     gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i + 1]], 0.0) - offset);
    //     // normal = interpolated_normals[tri_index + i + 1];
    //     EmitVertex();

    //     gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i + 2]], 0.0) - offset);
    //     // normal = interpolated_normals[tri_index + i + 2];
    //     EmitVertex();
    // }
    // EndPrimitive();

    gl_PointSize = gl_in[0].gl_PointSize;
    // gl_Position = proj * view * model * vec4(vec4(float(grid_pos.x) / width, float(grid_pos.y) / height, float(grid_pos.z) / depth, 0.0) - offset);
    gl_Position = proj * view * model * vec4(pos - offset, 0.0);
    EmitVertex();
    // gl_Position = proj * view * model * vec4(vec4(pos + vec3(0.01, 0.0, 0.0), 0.0) - offset);
    // EmitVertex();
    // gl_Position = proj * view * model * vec4(vec4(pos + vec3(0.01, 0.01, 0.0), 0.0) - offset);
    // EmitVertex();
    EndPrimitive();
}