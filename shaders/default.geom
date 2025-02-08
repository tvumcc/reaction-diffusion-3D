#version 460 core
layout (points) in;
layout (triangle_strip, max_vertices = 15) out;
// layout (points, max_vertices = 12) out;
// layout (points, max_vertices = 1) out;

layout (binding = 1, std430) readonly buffer ssbo1 {
    int edge_table[256];
};

layout (binding = 2, std430) readonly buffer ssbo2 {
    int vertex_table[24];
};

layout (binding = 3, std430) readonly buffer ssbo3 {
    int triangle_table[4096];
};

uniform float threshold;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform float width;
uniform float height;
uniform float depth;

out vec3 normal;
out vec3 FragPos;

uniform sampler3D grid_tex;
flat out ivec3 grid_pos;
flat out float tester;
flat out vec3 pos;

void main() {
    grid_pos = ivec3(gl_in[0].gl_Position.x * width, gl_in[0].gl_Position.y * height, gl_in[0].gl_Position.z * depth);
    vec4 offset = vec4(0.5, 0.5, 0.5, 0.0);
    pos = gl_in[0].gl_Position.xyz;

    // if (grid_pos.x == int(width)-1 || grid_pos.y == int(height)-1 || grid_pos.z == int(depth)-1) {
    //     return;
    // }

    float shift = 1.0 / (width);

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
    vec3 interpolated_normals[12];


    int cube_index = 0;
    for (int i = 0; i < 8; i++) {
        if (texture(grid_tex, pos + shifts[i]).g < threshold) {
            cube_index |= (1 << i);
        }
    }

    int edge_mask = edge_table[cube_index];
    int tri_index = cube_index * 16;

    float mx = 0.0;


    // Interpolate edge vertices
    for (int i = 0; i < 12; i++) {
        if (((edge_mask >> i) & 1) == 1) {
            vec3 P1 = pos + shifts[vertex_table[i*2]];
            vec3 P2 = pos + shifts[vertex_table[i*2+1]];
            float V1 = texture(grid_tex, P1).g;
            float V2 = texture(grid_tex, P2).g;


            // interpolated[i] = (P1 + P2) / 2.0;

            // mx = max(mx, max(interpolated[i].x, max(interpolated[i].y, interpolated[i].z)));
            mx = max(mx, interpolated[i].x);

            // interpolated[i] = P1 + (threshold - V1) * (P2 - P1) / (V2 - V1);
            interpolated[i] = mix(P1, P2, (threshold - V1) / (V2 - V1));
            // interpolated_normals[i] = normalize(normals[vertex_table[i*2]] + (threshold - V1) * (normals[vertex_table[i*2+1]] - normals[vertex_table[i*2]]) / (V2 - V1));
            // interpolated_normals[i] = (normals[vertex_table[i*2+1]] + normals[vertex_table[i*2]]) / 2.0;
        }
    }

    // Generate vertex normals
    for (int i = 0; i < 12; i++) {
        vec3 P = interpolated[i]; 
        float delta = shift;

        normals[i].x = texture(grid_tex, P + vec3(delta, 0.0, 0.0)).g - texture(grid_tex, P - vec3(delta, 0.0, 0.0)).g;
        normals[i].y = texture(grid_tex, P + vec3(0.0, delta, 0.0)).g - texture(grid_tex, P - vec3(0.0, delta, 0.0)).g;
        normals[i].z = texture(grid_tex, P + vec3(0.0, 0.0, delta)).g - texture(grid_tex, P - vec3(0.0, 0.0, delta)).g;
        normals[i] = normalize(normals[i]);
    }


    for (int i = 0; i < 16; i += 3) {
        if (triangle_table[tri_index + i] == -1) break;

        vec3 vertex1 = interpolated[triangle_table[tri_index + i]];
        vec3 vertex2 = interpolated[triangle_table[tri_index + i + 1]];
        vec3 vertex3 = interpolated[triangle_table[tri_index + i + 2]];

        // normal = normalize(cross(vertex2 - vertex1, vertex3 - vertex1));
        // if (dot(normal, normals[triangle_table[tri_index + i]]) < 0) {
        //     normal = -normal;
        // }
        gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i]], 1.0) - offset);
        normal = normals[triangle_table[tri_index + i]];
        FragPos = vec3(model * (vec4(interpolated[triangle_table[tri_index + i]], 1.0) - offset));
        EmitVertex();

        gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i + 1]], 1.0) - offset);
        normal = normals[triangle_table[tri_index + i + 1]];
        FragPos = vec3(model * (vec4(interpolated[triangle_table[tri_index + i + 1]], 1.0) - offset));
        EmitVertex();

        gl_Position = proj * view * model * (vec4(interpolated[triangle_table[tri_index + i + 2]], 1.0) - offset);
        normal = normals[triangle_table[tri_index + i + 2]];
        FragPos = vec3(model * (vec4(interpolated[triangle_table[tri_index + i + 2]], 1.0) - offset));
        EmitVertex();


        EndPrimitive();
    }


    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.0, 0.0, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.0, 1.0 / width, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(1.0 / width, 0.0, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(1.0 / width, 1.0 / width, 0.0, 0.0) - offset);
    // EmitVertex();
    // EndPrimitive();

    // for (int i = 0; i < 12; i++) {
    //     gl_Position = proj * view * model * (vec4(interpolated[i], 1.0) - offset);
    //     gl_PointSize = gl_in[0].gl_PointSize;
    //     EmitVertex();
    //     EndPrimitive();
    // }
    // gl_Position = proj * view * model * (vec4(pos, 1.0) - offset);
    // gl_PointSize = gl_in[0].gl_PointSize;
    // EmitVertex();
    // EndPrimitive();
}