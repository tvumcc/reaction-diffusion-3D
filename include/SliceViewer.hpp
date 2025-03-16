#pragma once
#include <glad/glad.h>

#include "Shader.hpp"
#include "Simulator.hpp"

namespace RD3D {
    /**
     * Manages the rendering of a 2D slice of the 3D grid.
     */
    class SliceViewer {
    public:
        SliceViewer(int grid_resolution);

        void render(int grid_resolution, GLuint grid_texture);
        void resize(int grid_resolution);
        void draw_gui(Simulator* simulator, int grid_resolution, int ui_sidebar_width);
    private:
        Shader shader;

        GLuint slice_texture;   
        GLuint slice_fbo;
        GLuint slice_vbo;
        GLuint slice_vao;

        int slice_depth = 0;

        void init_buffers(int grid_resolution);
    };
}