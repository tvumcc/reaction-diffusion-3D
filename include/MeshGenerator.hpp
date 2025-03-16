#pragma once
#include <glad/glad.h>

#include "Shader.hpp"
#include "OrbitalCamera.hpp"

#include <vector>

namespace RD3D {
    /**
     * A utility struct for use in the Marching Cubes Shader to write vertices from a
     * compute shader to a vertex array.
     */
    struct MarchingCubeVertex {
        alignas(8) glm::vec3 pos;
        alignas(8) glm::vec3 normal;
    };

    class MeshGenerator {
    public:
        MeshGenerator(int grid_resolution);

        void generate(int grid_resolution, GLuint grid_texture);
        void resize(int grid_resolution);
        void draw(OrbitalCamera& camera, int grid_resolution);
        void export_to_obj(int grid_resolution);

        void draw_gui(int grid_resolution);
    private:
        ComputeShader marching_cubes_shader;
        Shader mesh_shader;

        GLuint mesh_vbo;
        GLuint mesh_vao;
        float threshold = 0.2f;

        void init_buffers(int grid_resolution);
        void init_marching_cubes_tables();
    };
}