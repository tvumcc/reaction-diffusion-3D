#include <glad/glad.h>
#include <tiny_obj_loader.h>
#define VOXELIZER_IMPLEMENTATION
#include <voxelizer/voxelizer.h>

#include "grid.hpp"
#include "marching_cubes.hpp"

#include <iostream>

Grid::Grid() 
    : integration_shader("shaders/reaction_diffusion.glsl"),
      marching_cubes_shader("shaders/marching_cubes.glsl"),
      mesh_shader("shaders/default.vert", "shaders/default.frag"),
      slice_shader("shaders/slice.vert", "shaders/slice.frag")
{
    this->feed_rate = 0.035f;
    this->kill_rate = 0.065f;
    this->diffusion_u = 0.08f;
    this->diffusion_v = 0.04f;
    this->time_step = 0.55f;
    this->space_step = 1.00f;
    this->paused = true;
    this->integration_dispatches_per_frame = 1;

    this->grid_resolution = 64;
    this->mesh_resolution = 64;
    this->threshold = 0.2f;
    this->wireframe = false;

    this->slice_depth = 0;

    this->data = std::vector<float>(4 * grid_resolution * grid_resolution * grid_resolution);

    init_grid_texture();
    init_slice_buffers();
    init_mesh_buffers();
    init_marching_cubes_tables();
}

void Grid::gen_initial_conditions(std::vector<glm::vec3> dots) {
	for (int i = 0; i < grid_resolution; i++) {
		for (int j = 0; j < grid_resolution; j++) {
			for (int k = 0; k < grid_resolution; k++) {
				bool is_dot = false;
				for (auto dot : dots) {
					if ((i-(int)dot.x)*(i-(int)dot.x) + (j-(int)dot.y)*(j-(int)dot.y) + (k-(int)dot.z)*(k-(int)dot.z) <= 1*1) {
						is_dot = true;
						break;
					}
				}

				size_t idx = 4 * (i + j * grid_resolution + k * (grid_resolution * grid_resolution));
				data[idx] = 1.0f;
				data[idx + 1] = is_dot ? 1.0f : 0.0f;
			}
		}	
	}	
}

void Grid::gen_boundary_conditions(std::string obj_file_path) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;

    if (!reader.ParseFromFile(obj_file_path.c_str(), reader_config)) {
        if (!reader.Error().empty())
            std::cerr << "[ERROR] TinyObjReader: " << reader.Error();
        exit(1);
    }
    if (!reader.Warning().empty())
        std::cout << "[WARNING] TinyObjReader: " << reader.Warning();

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
	float res = 1.0 / grid_resolution;
	float precision = 0.01;
	unsigned int* voxels;

    for (int s = 0; s < shapes.size(); s++) { // Loop through shapes
        vx_mesh_t* mesh = vx_mesh_alloc(shapes[s].mesh.num_face_vertices.size(), shapes[s].mesh.indices.size());

		for (int v = 0; v < attrib.vertices.size() / 3; v++) {
			mesh->vertices[v].x = attrib.vertices[3 * size_t(v) + 0];
			mesh->vertices[v].y = attrib.vertices[3 * size_t(v) + 1];
			mesh->vertices[v].z = attrib.vertices[3 * size_t(v) + 2];
		}

        for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++) {
            mesh->indices[f] = shapes[s].mesh.indices[f].vertex_index;
        }

		voxels = vx_voxelize_snap_3dgrid(mesh, grid_resolution, grid_resolution, grid_resolution);
		break;
	}

	for (int i = 0; i < grid_resolution; i++) {
		for (int j = 0; j < grid_resolution; j++) {
			for (int k = 0; k < grid_resolution; k++) {
				size_t idx = i + j * grid_resolution + k * (grid_resolution * grid_resolution);
				data[4 * idx + 2] = voxels[idx] != 0 ? 1.0f : 0.0f; // Reset only the boundary condition component of the 3D grid
			}
		}
	}

}

void Grid::clear_boundary_conditions() {
	for (int i = 0; i < grid_resolution; i++) {
		for (int j = 0; j < grid_resolution; j++) {
			for (int k = 0; k < grid_resolution; k++) {
				size_t idx = 4 * (i + j * grid_resolution + k * (grid_resolution * grid_resolution)) + 2;
				data[idx] = 0.0f; // Reset only the boundary condition component of the 3D grid
			}
		}
	}	
}

void Grid::load_data_to_texture() {
	glBindTexture(GL_TEXTURE_3D, this->grid_texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, data.data());
	glBindImageTexture(0, grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void Grid::resize() {
	data = std::vector<float>(4 * grid_resolution * grid_resolution * grid_resolution, 0.0f);

	gen_initial_conditions(std::vector<glm::vec3>());
	clear_boundary_conditions();

	glBindTexture(GL_TEXTURE_3D, grid_texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, data.data());

	glBindTexture(GL_TEXTURE_2D, slice_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, grid_resolution, grid_resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
	glBufferData(GL_ARRAY_BUFFER, 15 * (grid_resolution / 2) * (grid_resolution / 2) * (grid_resolution / 2) * sizeof(MarchingCubeVertex), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_vbo);
}

void Grid::set_shader_uniforms() {
    // Integration Shader
	integration_shader.bind();
	integration_shader.set_int("grid_resolution", grid_resolution);
    integration_shader.set_float("Du", diffusion_u);
    integration_shader.set_float("Dv", diffusion_v);
    integration_shader.set_float("F", feed_rate);
    integration_shader.set_float("k", kill_rate);
	integration_shader.set_float("time_step", time_step);
	integration_shader.set_float("space_step", space_step);
	integration_shader.set_bool("paused", paused);

    // Marching Cubes Shader
	marching_cubes_shader.bind();
	marching_cubes_shader.set_float("grid_resolution", (float)grid_resolution);
    marching_cubes_shader.set_float("threshold", threshold);
	marching_cubes_shader.set_int("grid_tex", 0);
	
    // Slice Shader
    slice_shader.bind();
    slice_shader.set_int("slice_depth", slice_depth);
    slice_shader.set_int("grid_resolution", grid_resolution);
    slice_shader.set_int("grid_tex", 0);
}

void Grid::integration_compute_dispatch() {
    integration_shader.bind();
    for (int i = 0; i < integration_dispatches_per_frame; i++) {
        glDispatchCompute(grid_resolution, grid_resolution, grid_resolution);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void Grid::marching_cubes_compute_dispatch() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, grid_texture);
    marching_cubes_shader.bind();
    glDispatchCompute(grid_resolution / 2, grid_resolution / 2, grid_resolution / 2);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void Grid::draw_mesh() {
    mesh_shader.bind();
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);	
    glEnable(GL_CULL_FACE);
    glBindVertexArray(mesh_vao);
    glDrawArrays(GL_TRIANGLES, 0, 15 * grid_resolution * grid_resolution * grid_resolution);
}

void Grid::draw_slice() {
    glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	

    slice_shader.bind();
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, grid_texture);
    glViewport(0, 0, grid_resolution, grid_resolution);
    glBindVertexArray(slice_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Grid::init_grid_texture() {
	glGenTextures(1, &grid_texture);
	glBindTexture(GL_TEXTURE_3D, grid_texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	std::vector<glm::vec3> dots = {glm::vec3(grid_resolution / 2.0, grid_resolution / 2.0, grid_resolution / 2.0)};
    gen_initial_conditions(dots);
    load_data_to_texture();
}

void Grid::init_slice_buffers() {
    float vertices[] = {
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,    // top left 
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,    // top right
    };

	// Initialize the FBO for the cross section viewer
	glGenFramebuffers(1, &slice_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo);

	// Initialize the texture for the cross section viewer
	glGenTextures(1, &slice_texture);
	glBindTexture(GL_TEXTURE_2D, slice_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, grid_resolution, grid_resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, slice_texture, 0);

	glGenVertexArrays(1, &slice_vao);
	glBindVertexArray(slice_vao);

	glGenBuffers(1, &slice_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, slice_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Grid::init_mesh_buffers() {
	glGenVertexArrays(1, &mesh_vao);
	glBindVertexArray(mesh_vao);

	glGenBuffers(1, &mesh_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
	glBufferData(GL_ARRAY_BUFFER, 15 * (grid_resolution / 2) * (grid_resolution / 2) * (grid_resolution / 2) * sizeof(MarchingCubeVertex), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_vbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MarchingCubeVertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MarchingCubeVertex), (void*)offsetof(MarchingCubeVertex, normal));
	glEnableVertexAttribArray(1);
}

void Grid::init_marching_cubes_tables() {
	unsigned int edge_table_ssbo;
	glGenBuffers(1, &edge_table_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edge_table_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, edge_table.size() * sizeof(int), edge_table.data(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, edge_table_ssbo);

	unsigned int vertex_table_ssbo;
	glGenBuffers(1, &vertex_table_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_table_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, vertex_table.size() * sizeof(int), vertex_table.data(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vertex_table_ssbo);

	unsigned int triangle_table_ssbo;
	glGenBuffers(1, &triangle_table_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangle_table_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle_table.size() * sizeof(int), triangle_table.data(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triangle_table_ssbo);
}