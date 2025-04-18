#include <imgui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#define VOXELIZER_IMPLEMENTATION
#include <voxelizer/voxelizer.h>
#include <nfd.h>

#include "Simulator.hpp"
#include "Boundary.hpp"
#include "OrbitalCamera.hpp"

#include <iostream>

using namespace RD3D;

Boundary::Boundary() :
    boundary_shader("shaders/boundary.vert", "shaders/boundary.frag"),
    grid_boundary_mesh("assets/cube.obj")
{
}

/**
 * Import a mesh from a .obj file for use in defining the boundary conditions of the simulation.
 */
void Boundary::import_boundary_mesh() {
    nfdchar_t *outPath = NULL;		
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

    if (outPath != NULL) {
        boundary_mesh = std::make_unique<Mesh>(outPath);
        boundary_obj_path = outPath;
    }
}

/**
 * Gets rid of the boundary mesh from the state but does not remove the boundary values in the grid.
 */
void Boundary::clear_boundary_mesh() {
    boundary_mesh = nullptr;
    boundary_obj_path = "";
}

/**
 * Uses the currently loaded boundary mesh to place corresponding boundary values in the grid.
 * This will also erase all current boundary values and reset the simulation.
 */
void Boundary::voxelize_boundary() {
	if (boundary_obj_path.empty()) return;
	clear_boundary();

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;

    if (!reader.ParseFromFile(boundary_obj_path.c_str(), reader_config)) {
        if (!reader.Error().empty())
            std::cerr << "[ERROR] TinyObjReader: " << reader.Error();
        exit(1);
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
	float res = 1.0 / simulator->grid_resolution;
	float precision = 0.01;
	vx_point_cloud_t* voxels;

    for (int s = 0; s < shapes.size(); s++) {
        vx_mesh_t* mesh = vx_mesh_alloc(shapes[s].mesh.num_face_vertices.size(), shapes[s].mesh.indices.size());

		for (int v = 0; v < attrib.vertices.size() / 3; v++) {
			glm::vec3 vertex = glm::vec3(
				attrib.vertices[3 * size_t(v) + 0],
				attrib.vertices[3 * size_t(v) + 1],
				attrib.vertices[3 * size_t(v) + 2]
			);

			mesh->vertices[v].x = vertex.x;
			mesh->vertices[v].y = vertex.y;
			mesh->vertices[v].z = vertex.z;
		}

        for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++) {
            mesh->indices[f] = shapes[s].mesh.indices[f].vertex_index;
        }

		voxels = vx_voxelize_pc(mesh, 1.0 / simulator->grid_resolution, 1.0 / simulator->grid_resolution, 1.0 / simulator->grid_resolution, 1.0 / simulator->grid_resolution / 10.0);
		break;
	}

	for (int i = 0; i < voxels->nvertices; i++) {
		int x = ((int)((boundary_scale * voxels->vertices[i].x + boundary_offset.x) * simulator->grid_resolution)) + (simulator->grid_resolution / 2);
		int y = ((int)((boundary_scale * voxels->vertices[i].y + boundary_offset.y) * simulator->grid_resolution)) + (simulator->grid_resolution / 2);
		int z = ((int)((boundary_scale * voxels->vertices[i].z + boundary_offset.z) * simulator->grid_resolution)) + (simulator->grid_resolution / 2);

		if (x >= 0 && x < simulator->grid_resolution && y >= 0 && y < simulator->grid_resolution && z >= 0 && z < simulator->grid_resolution) {
			int idx = x + y * simulator->grid_resolution + z * (simulator->grid_resolution * simulator->grid_resolution);
			simulator->grid[4 * idx + 2] = 1.0;
		}
	}

	simulator->load_data_to_texture();	
}

/**
 * Clears all of the boundary values from the grid.
 */
void Boundary::clear_boundary() {
	for (int i = 0; i < simulator->grid_resolution; i++) {
		for (int j = 0; j < simulator->grid_resolution; j++) {
			for (int k = 0; k < simulator->grid_resolution; k++) {
				size_t idx = 4 * (i + j * simulator->grid_resolution + k * (simulator->grid_resolution * simulator->grid_resolution)) + 2;
				simulator->grid[idx] = 0.0f; // Reset only the boundary component of the 3D grid
			}
		}
	}	

	simulator->load_data_to_texture();
}

/**
 * Thickens the boundary values in all directions.
 */
void Boundary::thicken_boundary() {
	auto get_grid_cell = [](Simulator* sim, int x, int y, int z){
		size_t idx = 4 * (z + y * sim->grid_resolution + x * (sim->grid_resolution * sim->grid_resolution)) + 2;
		return &sim->grid[idx];
	};

	int dx[3] = {-1, 0, 1};
	int dy[3] = {-1, 0, 1};
	int dz[3] = {-1, 0, 1};

	for (int i = 0; i < simulator->grid_resolution; i++) {
		for (int j = 0; j < simulator->grid_resolution; j++) {
			for (int k = 0; k < simulator->grid_resolution; k++) {
				size_t idx = 4 * (i + j * simulator->grid_resolution + k * (simulator->grid_resolution * simulator->grid_resolution)) + 2;
				if (*get_grid_cell(simulator, k, j, i) == 1.0f) {
					for (int a = 0; a < 3; a++) {
						for (int b = 0; b < 3; b++) {
							for (int c = 0; c < 3; c++) {
								int nx = k + dx[a];
								int ny = j + dy[b];
								int nz = i + dz[c];

								if (nx >= 0 && nx < simulator->grid_resolution && ny >= 0 && ny < simulator->grid_resolution && nz >= 0 && nz < simulator->grid_resolution) {
									auto p = get_grid_cell(simulator, nx, ny, nz);	
									if (*p == 0.0f) *p = 0.5f;
								}
							}
						}
					}
				}
			}
		}
	}	

	for (int i = 0; i < simulator->grid_resolution; i++) {
		for (int j = 0; j < simulator->grid_resolution; j++) {
			for (int k = 0; k < simulator->grid_resolution; k++) {
				auto p = get_grid_cell(simulator, k, j, i);
				if (*p == 0.5f) *p = 1.0f;	
			}
		}
	}	

	simulator->load_data_to_texture();
}

/**
 * Turns all boundary value cells into empty cells and vice versa.
 */
void Boundary::invert_boundary() {
	auto get_grid_cell = [](Simulator* sim, int x, int y, int z){
		size_t idx = 4 * (z + y * sim->grid_resolution + x * (sim->grid_resolution * sim->grid_resolution)) + 2;
		return &sim->grid[idx];
	};

	for (int i = 0; i < simulator->grid_resolution; i++) {
		for (int j = 0; j < simulator->grid_resolution; j++) {
			for (int k = 0; k < simulator->grid_resolution; k++) {
				auto p = get_grid_cell(simulator, k, j, i);
				if (*p == 1.0f) *p = 0.0f;	
				else *p = 1.0f;
			}
		}
	}	

	simulator->load_data_to_texture();
}

/**
 * Draws the currently selected boundary mesh in 3D space.
 * 
 * @param camera The camera to render in the perspective of
 */
void Boundary::draw_boundary_mesh(OrbitalCamera& camera) {
	if (boundary_mesh) {
		boundary_shader.bind();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, boundary_offset);
		model = glm::scale(model, glm::vec3(boundary_scale));
		boundary_shader.set_mat4x4("model", model);
		boundary_shader.set_mat4x4("view_proj", camera.get_view_projection_matrix());
		boundary_shader.set_vec3("object_color", glm::vec3(1.0f, 0.0f, 0.0f));
		boundary_shader.set_float("opacity", boundary_mesh_opacity);

		boundary_mesh->draw(boundary_shader, GL_TRIANGLES);
	}
}

/**
 * Draws the cube that highlights the unchanging boundaries of the 3D grid.
 * 
 * @param camera The camera to render in the perspective of
 */
void Boundary::draw_grid_boundary_mesh(OrbitalCamera& camera) {
	boundary_shader.bind();
	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	boundary_shader.set_mat4x4("model", model);
	boundary_shader.set_mat4x4("view_proj", camera.get_view_projection_matrix());
	boundary_shader.set_vec3("object_color", glm::vec3(0.0f, 0.0f, 1.0f));
	boundary_shader.set_float("opacity", grid_cube_opacity);

	grid_boundary_mesh.draw(boundary_shader, GL_TRIANGLES);
}

/**
 * Draw the GUI section that allows for manipulation of the simulation's boundaries.
 */
void Boundary::draw_gui() {
	if (ImGui::Button("Import")) import_boundary_mesh();
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		clear_boundary_mesh();
		clear_boundary();
	}
	ImGui::SameLine();
	if (ImGui::Button("Voxelize")) voxelize_boundary();
	ImGui::SameLine();
	if (ImGui::Button("Reset Transforms")) {
		boundary_offset = glm::vec3(0.0f, 0.0f, 0.0f);
		boundary_scale = 1.0f;
	}
	if (ImGui::Button("Thicken")) thicken_boundary();
	ImGui::SameLine();
	if (ImGui::Button("Invert")) invert_boundary();

	ImGui::SliderFloat3("Mesh Offset", glm::value_ptr(boundary_offset), -1.0f, 1.0f);
	ImGui::SliderFloat("Mesh Scale", &boundary_scale, 0.0f, 2.0f);

	ImGui::SliderFloat("Grid Cube Opacity", &grid_cube_opacity, 0.0f, 1.0f);
	ImGui::SliderFloat("Boundary Mesh Opacity", &boundary_mesh_opacity, 0.0f, 1.0f);
}