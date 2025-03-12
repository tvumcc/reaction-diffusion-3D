#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#define VOXELIZER_IMPLEMENTATION
#include <voxelizer/voxelizer.h>

#include "grid.hpp"
#include "marching_cubes.hpp"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <unordered_map>

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

    this->slice_depth = grid_resolution / 2;

    this->data = std::vector<float>(4 * grid_resolution * grid_resolution * grid_resolution);

    init_grid_texture();
    init_slice_buffers();
    init_mesh_buffers();
    init_marching_cubes_tables();
}

/**
 * Generate initial conditions in the 3D data array by placing spherical dots
 * of radius 1 at the specified coordinates.
 * 
 * @param dots The 3D coordinates at which to place initial value spheres
 */
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

/**
 * Generate boundary conditions in the 3D data array by voxelizing the given
 * 3D model from the .obj file.
 * 
 * @param obj_file_path The .obj file containing the 3D model to voxelize and set as boundary condition
 */
void Grid::gen_boundary_conditions(std::string obj_file_path, glm::vec3 offset, float scale) {
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
	vx_point_cloud_t* voxels;

    for (int s = 0; s < shapes.size(); s++) { // Loop through shapes
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

		voxels = vx_voxelize_pc(mesh, 1.0 / grid_resolution, 1.0 / grid_resolution, 1.0 / grid_resolution, 1.0 / grid_resolution / 10.0);
		break;
	}

	for (int i = 0; i < voxels->nvertices; i++) {
		int x = ((int)((scale * voxels->vertices[i].x + offset.x) * grid_resolution)) + (grid_resolution / 2);
		int y = ((int)((scale * voxels->vertices[i].y + offset.y) * grid_resolution)) + (grid_resolution / 2);
		int z = ((int)((scale * voxels->vertices[i].z + offset.z) * grid_resolution)) + (grid_resolution / 2);

		if (x >= 0 && x < grid_resolution && y >= 0 && y < grid_resolution && z >= 0 && z < grid_resolution) {
			int idx = x + y * grid_resolution + z * (grid_resolution * grid_resolution);
			data[4 * idx + 2] = 1.0;
		}

	}
}

/**
 * Reset the boundary conditions to the edges of the 3D data array.
 */
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

/**
 * Export the current state of the mesh to a triangulated .obj file
 */
void Grid::export_mesh_to_obj() {
	// Read in mesh data from the vertex array
	glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
	size_t sz = 15 * (grid_resolution / 2) * (grid_resolution / 2) * (grid_resolution / 2);
	MarchingCubeVertex* ptr = new MarchingCubeVertex[sz];
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sz * sizeof(MarchingCubeVertex), ptr);
	
    std::ofstream objFile("out.obj");
    if (!objFile.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        delete[] ptr;
        return;
    }

	std::vector<glm::vec3> distinctPositions;
	std::vector<glm::vec3> distinctNormals;
	std::vector<int> faceIndices;

	std::unordered_map<glm::vec3, int, std::hash<glm::vec3>> positionMap;
    std::unordered_map<glm::vec3, int, std::hash<glm::vec3>> normalMap;

    for (size_t i = 0; i < sz; i++) {
        // Extract position and normal using GLM
        glm::vec3 pos = ptr[i].pos - glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 norm = ptr[i].normal;

        // Check if position is already in the distinctPositions
        if (positionMap.count(pos) == 0) {
            positionMap[pos] = distinctPositions.size(); // Index of the new position
            distinctPositions.push_back(pos);
        }

        // Check if normal is already in the distinctNormals
        if (normalMap.count(norm) == 0) {
            normalMap[norm] = distinctNormals.size(); // Index of the new normal
            distinctNormals.push_back(norm);
        }

        // Add face indices based on the position and normal indices
        faceIndices.push_back(positionMap[pos]);
        faceIndices.push_back(normalMap[norm]);
    }

	for (int i = 0; i < distinctPositions.size(); i++) {
		objFile << "v " << distinctPositions[i].x << " " << distinctPositions[i].y << " " << distinctPositions[i].z << "\n";
	}

	for (int i = 0; i < distinctNormals.size(); i++) {
		objFile << "vn " << distinctNormals[i].x << " " << distinctNormals[i].y << " " << distinctNormals[i].z << "\n";
	}

	for (int i = 0; i < faceIndices.size(); i += 6) {
		glm::vec3 A = distinctPositions[faceIndices[i]];
		glm::vec3 B = distinctPositions[faceIndices[i+2]];
		glm::vec3 C = distinctPositions[faceIndices[i+4]];
		float a = glm::length(B - C);
		float b = glm::length(A - C);
		float c = glm::length(A - B);
		float s = 0.5 * (a + b + c);
		float area = sqrt(s * (s - a) * (s - b) * (s - c));
		if (area <= 0) continue;

		objFile << "f " << faceIndices[i]+1 << "//" << faceIndices[i+1]+1 << " "; 
		objFile << faceIndices[i+2]+1 << "//" << faceIndices[i+3]+1 << " "; 
		objFile << faceIndices[i+4]+1 << "//" << faceIndices[i+5]+1 << "\n"; 
	}

    objFile.close();
    std::cout << "Exported successfully!" << std::endl;

    delete[] ptr;
}

/**
 * Write the values of the 3D data array on the CPU to the 3D texture on the GPU.
 */
void Grid::load_data_to_texture() {
	glBindTexture(GL_TEXTURE_3D, this->grid_texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, data.data());
	glBindImageTexture(0, grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
}

/**
 * Adjust the dimensions of the 3D data array, 3D grid texture, 2D slice texture,
 * and mesh vertex array to match that of the grid's set resolution. 
 */
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

/**
 * Enables the brush so that initial value spheres can be added to the 3D grid texture on the GPU.
 * 
 * @param x The x position of the brush stroke
 * @param y The y position of the brush stroke
 */
void Grid::enable_brush(int x, int y, int z) {
	brush_enabled = true;
	brush_x = x;
	brush_y = y;
	brush_z = z;
}

/**
 * Disables the brush
 */
void Grid::disable_brush() {
	brush_enabled = false;
}

/**
 * Set uniforms for the integration shader, Marching Cubes shader, and slice shader.
 */
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
	integration_shader.set_bool("brush_enabled", brush_enabled);
	integration_shader.set_int("brush_x", brush_x);
	integration_shader.set_int("brush_y", brush_y);
	integration_shader.set_int("brush_z", brush_z);

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

/**
 * Compute as many time steps in the simulation as specified by integration_dispatches_per_frame
 * for the Gray-Scott Reaction Diffusion equations.
 */
void Grid::integration_compute_dispatch() {
    integration_shader.bind();
    for (int i = 0; i < integration_dispatches_per_frame; i++) {
        glDispatchCompute(grid_resolution, grid_resolution, grid_resolution);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

/**
 * Run the Marching Cubes Algorithm to convert the scalar field generated by the Gray-Scott
 * equations into a triangular mesh.
 */
void Grid::marching_cubes_compute_dispatch() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, grid_texture);
    marching_cubes_shader.bind();
    glDispatchCompute(grid_resolution / 2, grid_resolution / 2, grid_resolution / 2);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Draw the triangular mesh generated by the Marching Cubes algorithm.
 */
void Grid::draw_mesh() {
    mesh_shader.bind();
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);	
    glEnable(GL_CULL_FACE);
    glBindVertexArray(mesh_vao);
    glDrawArrays(GL_TRIANGLES, 0, 15 * grid_resolution * grid_resolution * grid_resolution);
}

/**
 * Draw the slice of the 3D grid texture as specified by slice_depth.
 */
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

/**
 * Initialize the 3D grid texture and place an initial value sphere
 *  of radius 1 at the center.
 */
void Grid::init_grid_texture() {
	glGenTextures(1, &grid_texture);
	glBindTexture(GL_TEXTURE_3D, grid_texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    gen_initial_conditions(std::vector<glm::vec3>());
    load_data_to_texture();
}

/**
 * Initialize the vertex buffers and 2D texture for rendering the slice viewer.
 */
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

/**
 * Initialize the vertex buffers for the trianglular mesh as well as the
 * SSBO that will be bound to the same location for use by the Marching Cubes
 * compute shader.
 */
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

/**
 * Set up the SSBO look-up tables for the Marching Cubes Algorithm so that
 * they can be used by the compute shader.
 */
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