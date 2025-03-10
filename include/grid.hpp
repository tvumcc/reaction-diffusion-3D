#pragma once
#include "shader.hpp"

#include <vector>
#include <string>

struct MarchingCubeVertex {
	alignas(8) glm::vec3 pos;
	alignas(8) glm::vec3 normal;
};

class Grid {
public:
    // Gray-Scott Reaction Diffusion Simulation settings
    float feed_rate;
    float kill_rate;
    float diffusion_u;
    float diffusion_v;
    float time_step;
    float space_step;
    bool paused;
    int integration_dispatches_per_frame;

    // Grid and Mesh OpenGL objects and settings
    unsigned int grid_texture;
    unsigned int mesh_vbo;
    unsigned int mesh_vao;
    int grid_resolution;
    int mesh_resolution;
    float threshold;
    bool wireframe;

    // Cross Section OpenGL objects and settings
    int slice_depth;
    unsigned int slice_fbo;
    unsigned int slice_texture;
    unsigned int slice_vbo;
    unsigned int slice_vao;

    ComputeShader integration_shader;
    ComputeShader marching_cubes_shader;
    Shader mesh_shader;
    Shader slice_shader;

    std::vector<float> data;

    Grid();

    void gen_initial_conditions(std::vector<glm::vec3> dots);
    void gen_boundary_conditions(std::string obj_file_path);
    void clear_boundary_conditions();
    void load_data_to_texture();
    void resize();

    void set_shader_uniforms();
    void integration_compute_dispatch();
    void marching_cubes_compute_dispatch();
    void draw_slice();
    void draw_mesh();

    void init_grid();
    void init_grid_texture();
    void init_slice_buffers();
    void init_mesh_buffers();
    void init_marching_cubes_tables();
    void init_shaders();
private:
};