#pragma once
#include <glm/glm.hpp>

#include "Shader.hpp"
#include "Mesh.hpp"
#include "OrbitalCamera.hpp"

#include <vector>
#include <string>
#include <memory>

namespace RD3D {
    class Simulator;

    class Boundary {
    public:
        Boundary();

        void import_boundary_mesh();
        void clear_boundary_mesh();
        void voxelize_boundary();
        void clear_boundary();

        void draw_boundary_mesh(OrbitalCamera& camera);
        void draw_grid_boundary_mesh(OrbitalCamera& camera);

        void draw_gui();
    private:
        Simulator* simulator;

        Shader boundary_shader;
        Mesh grid_boundary_mesh;
        std::unique_ptr<Mesh> boundary_mesh;

        glm::vec3 boundary_offset = glm::vec3(0.0f, 0.0f, 0.0f);
        float boundary_scale = 1.0f;
        std::string boundary_obj_path;

        float grid_cube_opacity = 0.1f;
        float boundary_mesh_opacity = 0.3f;

        void set_simulator(Simulator* simulator);

        friend class Simulator;
    };
}