#pragma once
#include "shader.hpp"

#include <vector>

/**
 * A utility struct for use in the Marching Cubes Shader to write vertices from a
 * compute shader to a vertex array.
 */
struct MarchingCubeVertex {
	alignas(8) glm::vec3 pos;
	alignas(8) glm::vec3 normal;
};

/**
 * A class for managing Gray-Scott numerical simulation, initial conditions, 
 * boundary conditions, and slice viewing. 
 */
class Grid {
public:
    // Gray-Scott Reaction Diffusion Simulation settings
    float feed_rate; // The rate at which chemical U feeds off of chemical V
    float kill_rate; // The rate at which chemical V dies off
    float diffusion_u; // Diffusion constant of chemical U
    float diffusion_v; // Diffusion constant of chemical V
    float time_step; // The time step used by the Explicit Euler's Method of Integration
    float space_step; // The "difference in space" between grid cells. This is usually kept at 1.0
    bool paused; // If true, the simulation does not step through time, if false, the simulation progresses as normal
    int integration_dispatches_per_frame; // The number of time steps to perform for each frame of the application

    // Grid and Mesh OpenGL objects and settings
    GLuint grid_texture;
    GLuint mesh_vbo, mesh_vao;
    int grid_resolution; // Resolution of the 3D grid texture
    int mesh_resolution; // Resolution of the triangular mesh
    float threshold; // The isovalue at which the mesh is rendered
    bool wireframe; // If false, the mesh is rendered as wireframe triangles, if true, the triangles are filled instead

    // Cross Section OpenGL objects and settings
    GLuint slice_texture;
    GLuint slice_fbo, slice_vbo, slice_vao;
    int slice_depth; // The current slice of the 3D texture that is being viewed

    // Brush variables
    int brush_x, brush_y, brush_z; // The x and y position of the brush stroke
    bool brush_enabled; // If true, the brush is enabled, if false, it is not

    // Compute and Vert+Frag Shaders
    ComputeShader integration_shader; // Numerically integrates the Gray-Scott PDEs
    ComputeShader marching_cubes_shader; // Creates a triangular mesh from the Gray-Scott solution grid
    Shader mesh_shader; // Used to draw the mesh
    Shader slice_shader; // Used to draw the slice on the left UI sidebar

    std::vector<float> data; // 3D grid of vec4's where each cell is (u, v, boundary, 0)

    Grid();


    // Texture and Data Modification Functions

    void gen_initial_conditions(std::vector<glm::vec3> dots);
    void gen_boundary_conditions(std::string obj_file_path, glm::vec3 offset, float scale);
    void clear_boundary_conditions();
    void load_data_to_texture();
    void resize();
    void enable_brush(int x, int y, int z);
    void disable_brush();


    // Per frame Utility Functions

    void set_shader_uniforms();
    void integration_compute_dispatch();
    void marching_cubes_compute_dispatch();
    void draw_slice();
    void draw_mesh();


    // OpenGL Object Initialization Functions

    void init_grid_texture();
    void init_slice_buffers();
    void init_mesh_buffers();
    void init_marching_cubes_tables();
};