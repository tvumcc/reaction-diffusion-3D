#version 460 core
layout (points) in;
// layout (triangle_strip, max_vertices = 4) out;
layout (points, max_vertices = 1) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform float width;
uniform float height;
uniform float depth;

flat out ivec3 grid_pos;

void main() {
    grid_pos = ivec3(gl_in[0].gl_Position.x * width, gl_in[0].gl_Position.y * height, gl_in[0].gl_Position.z * depth);
    vec4 offset = vec4(0.5, 0.5, 0.5, 0.0);

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.0, 0.0, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.0, 0.05, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.05, 0.0, 0.0, 0.0) - offset);
    // EmitVertex();

    // gl_Position = proj * view * model * (gl_in[0].gl_Position + vec4(0.05, 0.05, 0.0, 0.0) - offset);
    // EmitVertex();

    gl_Position = proj * view * model * (gl_in[0].gl_Position - offset);
    gl_PointSize = gl_in[0].gl_PointSize;
    EmitVertex();

    EndPrimitive();
}