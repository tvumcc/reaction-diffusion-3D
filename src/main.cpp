#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <tiny_obj_loader.h>
#define VOXELIZER_IMPLEMENTATION
#include <voxelizer/voxelizer.h>

#include "shader.hpp"
#include "mesh.hpp"
#include "marching_cubes.hpp"

#include <iostream>
#include <string>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);
void cursor_pos_callback(GLFWwindow*, double x_pos, double y_pos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int WINDOW_WIDTH = 1100;
unsigned int WINDOW_HEIGHT = 800;

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
float pitch = 0.0f;
float yaw = 90.0f;
float radius = 1.0f;
float threshold = 0.2f;

unsigned int width = 128;
unsigned int height = 128;
unsigned int depth = 128;
unsigned int work_group_size = 8;

int alternate = 0;
unsigned int gui_width = 300;

float a = 0.035f;
float b = 0.065f;
float Du = 0.08f;
float Dv = 0.04f;
bool paused = true;
bool mouse = true;

double last_frame;

struct MarchingCubeVertex {
	alignas(8) glm::vec3 pos;
	alignas(8) glm::vec3 normal;
};

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Reaction Diffusion 3D", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	auto resize_window = [](GLFWwindow* window, int width, int height){
		WINDOW_WIDTH = width;
		WINDOW_HEIGHT = height;
		glViewport(0, 0, WINDOW_WIDTH - gui_width, WINDOW_HEIGHT);
	};
    glfwSetFramebufferSizeCallback(window, resize_window);
	resize_window(window, WINDOW_WIDTH, WINDOW_HEIGHT);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	if (mouse) 
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwWindowHint(GLFW_SAMPLES, 8);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);
	last_frame = glfwGetTime();

    // tinyobj::ObjReader reader;
    // tinyobj::ObjReaderConfig reader_config;

    // if (!reader.ParseFromFile("assets/bunny.obj", reader_config)) {
    //     if (!reader.Error().empty())
    //         std::cerr << "[ERROR] TinyObjReader: " << reader.Error();
    //     exit(1);
    // }
    // if (!reader.Warning().empty())
    //     std::cout << "[WARNING] TinyObjReader: " << reader.Warning();

    // auto& attrib = reader.GetAttrib();
    // auto& shapes = reader.GetShapes();
	// float res = 1.0 / width;
	// float precision = 0.01;
	// std::cout << "Shapes: " << shapes.size() << "\n";
	// unsigned int* voxels;

    // for (int s = 0; s < shapes.size(); s++) { // Loop through shapes
    //     vx_mesh_t* mesh = vx_mesh_alloc(shapes[s].mesh.num_face_vertices.size(), shapes[s].mesh.indices.size());

	// 	for (int v = 0; v < attrib.vertices.size() / 3; v++) {
	// 		mesh->vertices[v].x = attrib.vertices[3 * size_t(v) + 0];
	// 		mesh->vertices[v].y = attrib.vertices[3 * size_t(v) + 1];
	// 		mesh->vertices[v].z = attrib.vertices[3 * size_t(v) + 2];
	// 	}

    //     for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++) {
    //         mesh->indices[f] = shapes[s].mesh.indices[f].vertex_index;
    //     }

	// 	voxels = vx_voxelize_snap_3dgrid(mesh, width, height, depth);
	// 	break;
	// }

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	std::vector<glm::vec3> initial_dots;
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0 - 10, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0 + 10, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0 - 10, height / 2.0, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0 + 10, height / 2.0, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0, depth / 2.0 + 10));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0, depth / 2.0 - 10));

	std::vector<float> initial_conditions;
	for (int k = 0; k < depth; k++) {
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				bool is_dot = false;
				for (auto dot : initial_dots) {
					if ((i-(int)dot.x)*(i-(int)dot.x) + (j-(int)dot.y)*(j-(int)dot.y) + (k-(int)dot.z)*(k-(int)dot.z) <= 1*1) {
						is_dot = true;
						break;
					}
				}

				initial_conditions.push_back(1.0f);
				initial_conditions.push_back(is_dot ? 1.0f : 0.0f);
				size_t idx = i + j * width + k * (width * height);
				// initial_conditions.push_back(voxels[idx] != 0 ? 1.0f : 0.0f);
				initial_conditions.push_back(0.0f);
				initial_conditions.push_back(0.0f);
			}
		}
	}
	
	// int count = 0;
	// for (int i = 0; i < width * height * depth; i++) {
	// 	if (voxels[i] != 0) count++;
	// }
	// std::cout << "Voxels: " << count << "\n";

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
	glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	// std::vector<Vertex> rd_mesh_vertices;
	// for (int i = 0; i < width; i++) {
	// 	for (int j = 0; j < height; j++) {
	// 		for (int k = 0; k < depth; k++) {
	// 			Vertex vertex = {
	// 				glm::vec3(i / (float)width, j / (float)height, k / (float)depth),
	// 				glm::vec2(0.0, 0.0),
	// 				glm::vec3(0.0, 0.0, 0.0)
	// 			};
	// 			rd_mesh_vertices.push_back(vertex);
	// 		}
	// 	}
	// }
	// Mesh rd(rd_mesh_vertices);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// Shader shader("shaders/default.vert", "shaders/default.frag", "shaders/default.geom");
	Shader shader("shaders/default.vert", "shaders/default.frag");
    shader.bind();
	shader.set_int("grid_tex", 0);

	ComputeShader compute_shader("shaders/reaction_diffusion.glsl");
	compute_shader.bind();
	compute_shader.set_int("width", width);
	compute_shader.set_int("height", height);
	compute_shader.set_int("depth", depth);
	compute_shader.set_float("time_step", 0.55f);
	compute_shader.set_float("space_step", 1.00f);

	ComputeShader marching_cubes("shaders/marching_cubes.glsl");
	marching_cubes.bind();
	marching_cubes.set_float("width", (float)width);
	marching_cubes.set_float("height", (float)height);
	marching_cubes.set_float("depth", (float)depth);
	marching_cubes.set_int("grid_tex", 0);

	shader.bind();
	
	// will be bound as both a VBO and SSBO, for generating and rendering vertices
	unsigned int grid_vbo, grid_vao;
	glGenVertexArrays(1, &grid_vao);
	glBindVertexArray(grid_vao);

	glGenBuffers(1, &grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, 15 * (width / 2) * (height / 2) * (depth / 2) * sizeof(MarchingCubeVertex), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, grid_vbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MarchingCubeVertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MarchingCubeVertex), (void*)offsetof(MarchingCubeVertex, normal));
	glEnableVertexAttribArray(1);


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

	while (!glfwWindowShouldClose(window)) {
		double curr_frame = glfwGetTime();
		double fps = 1.0 / (curr_frame - last_frame);
		last_frame = curr_frame;

		process_input(window);
		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui Stuff Goes Here
		const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x - gui_width, main_viewport->WorkPos.y));
		ImGui::SetNextWindowSize(ImVec2(gui_width, main_viewport->WorkSize.y));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

		ImGui::Begin("Menu");
		ImGui::Text(("Current FPS: " + std::to_string((int)fps)).c_str());
		ImGui::SliderFloat("Feed Rate", &a, 0.0f, 0.1f);
		ImGui::SliderFloat("Kill Rate", &b, 0.0f, 0.1f);
		ImGui::Text(std::to_string(threshold).c_str());
		ImGui::Checkbox("Paused", &paused);
		if (ImGui::Button("Reset") || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
			glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
		}

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);
		ImGui::End();

		compute_shader.bind();
		compute_shader.set_float("Du", Du);
		compute_shader.set_float("Dv", Dv);
		compute_shader.set_float("F", a);
		compute_shader.set_float("k", b);
		compute_shader.set_bool("paused", paused);
		for (int i = 0; i < 3; i++) {
			glDispatchCompute(width / work_group_size, height / work_group_size, depth / work_group_size);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, texture);
		marching_cubes.bind();
		marching_cubes.set_float("threshold", threshold);
		glDispatchCompute((width / 2) / work_group_size, (height / 2) / work_group_size, (depth / 2) / work_group_size);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		// yaw += 0.5;

		camera_position = glm::vec3(
			position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
			position.y + radius * glm::sin(glm::radians(pitch)),
			position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
		);


		shader.bind();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, (float)(WINDOW_WIDTH - gui_width) / (float)WINDOW_HEIGHT, 0.01f, 100.0f);
		shader.set_mat4x4("model", model);
		shader.set_mat4x4("view", view);
		shader.set_mat4x4("proj", proj);
		shader.set_vec3("cam_pos", camera_position);

		glBindVertexArray(grid_vao);
		glDrawArrays(GL_TRIANGLES, 0, 15 * width * height * depth);
		// rd.draw(shader, GL_POINTS);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void process_input(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);	
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) threshold += 0.01f;
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) threshold -= 0.01f;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		mouse = true;
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		mouse = false;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		paused = !paused;
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
}

void cursor_pos_callback(GLFWwindow*, double x_pos, double y_pos) {
	static float last_x = (float)WINDOW_WIDTH / 2.0;
	static float last_y = (float)WINDOW_HEIGHT / 2.0;
	static bool first_mouse = true;

	if (first_mouse) {
		last_x = x_pos;
		last_y = y_pos;	
		first_mouse = false;
	}

	float x_offset = (float)x_pos - last_x;
	float y_offset = last_y - (float)y_pos;
	last_x = (float)x_pos;
	last_y = (float)y_pos;

	if (!mouse) {
		yaw += x_offset;	
		if (pitch + y_offset < 89.0f && pitch + y_offset > -89.0f) {
			pitch += y_offset;
		} 
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (radius + 0.1 * yoffset > 0.0f) {
		radius += 0.1 * yoffset;
	}
}