#pragma once
#include <glad/glad.h>

#include "Shader.hpp"
#include "Boundary.hpp"
#include "MeshGenerator.hpp"
#include "SliceViewer.hpp"

#include <vector>

namespace RD3D {
    class SliceViewer;

    /**
     * Manages the numerical simulation of the Gray-Scott Reaction Diffusion model
     * using the Finite Difference Method as well as the parameters that change the
     * behavior of the simulation.
     */
    class Simulator {
    public:
        GLuint grid_texture;
        int grid_resolution = 64;
        Boundary boundary;

        Simulator();

        void simulate_time_steps();
        void reset();
        void resize();
        void enable_brush(int x, int y, int z);
        void disable_brush();
        void toggle_pause();

        void draw_gui(MeshGenerator& mesh_generator, SliceViewer& slice_viewer);
    private:
        // Gray-Scott Reaction Diffusion Simulation settings
        // These settings give good immediate results without the user having to fine tune anything
        float feed_rate = 0.035f;
        float kill_rate = 0.065f;
        float diffusion_u = 0.08f;
        float diffusion_v = 0.04f;
        float time_step = 0.55f;;
        float space_step = 1.00f;
        int simulation_time_steps_per_frame = 1;
        bool paused = false;

        int brush_x = 0;
        int brush_y = 0;
        int brush_z = 0;
        bool brush_enabled = false;

        ComputeShader shader;
        std::vector<float> grid;

        void set_shader_uniforms();
        void load_data_to_texture();

        friend class Boundary;
    };
}