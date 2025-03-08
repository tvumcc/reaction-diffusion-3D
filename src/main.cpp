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
#include <nfd.h>

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
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

unsigned int WINDOW_WIDTH = 1100;
unsigned int WINDOW_HEIGHT = 800;

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
float pitch = 0.0f;
float yaw = 90.0f;
float radius = 1.0f;
float threshold = 0.2f;

unsigned int work_group_size = 1;
int grid_resolution = 128;
int mesh_resolution = 64;
int dispatches_per_frame = 3;
int slice_depth = 0;

int alternate = 0;
unsigned int gui_width = 300;

float a = 0.035f;
float b = 0.065f;
float Du = 0.08f;
float Dv = 0.04f;
bool paused = true;
bool mouse = true;
bool wireframe = false;
bool interpolation = true;

float vertices[] = {
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,   // top right
	 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom right
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom left
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,    // top left 
};

unsigned int indices[] = {
	0, 1, 2,
	2, 3, 0
};

double last_frame;
std::vector<float> fps_tracker;

std::vector<float> initial_conditions = std::vector<float>(grid_resolution * grid_resolution * grid_resolution * 4, 0.0f);

unsigned texture;
unsigned slice_fbo, slice_texture;
unsigned int grid_vbo, grid_vao;

struct MarchingCubeVertex {
	alignas(8) glm::vec3 pos;
	alignas(8) glm::vec3 normal;
};

void clear_boundary_conditions() {
	for (int i = 0; i < grid_resolution; i++) {
		for (int j = 0; j < grid_resolution; j++) {
			for (int k = 0; k < grid_resolution; k++) {
				size_t idx = 4 * (i + j * grid_resolution + k * (grid_resolution * grid_resolution)) + 2;
				initial_conditions[idx] = 0.0f; // Reset only the boundary condition component of the 3D grid
			}
		}
	}	
}

void initialize_boundary_conditions(std::string filename) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;

    if (!reader.ParseFromFile(filename.c_str(), reader_config)) {
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
				initial_conditions[4 * idx + 2] = voxels[idx] != 0 ? 1.0f : 0.0f; // Reset only the boundary condition component of the 3D grid
			}
		}
	}
}

void initialize_initial_conditions(std::vector<glm::vec3> dots) {
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
				initial_conditions[idx] = 1.0f;
				initial_conditions[idx + 1] = is_dot ? 1.0f : 0.0f;
			}
		}	
	}	
}

void load_initial_conditions_to_texture() {
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
	glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void resize_everything() {
	initial_conditions = std::vector<float>(4 * grid_resolution * grid_resolution * grid_resolution, 0.0f);

	initialize_initial_conditions(std::vector<glm::vec3>());
	clear_boundary_conditions();

	glBindTexture(GL_TEXTURE_3D, texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());

	glBindTexture(GL_TEXTURE_2D, slice_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, grid_resolution, grid_resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, 15 * (grid_resolution / 2) * (grid_resolution / 2) * (grid_resolution / 2) * sizeof(MarchingCubeVertex), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, grid_vbo);
}

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
	glfwSetKeyCallback(window, key_callback);
	if (mouse) 
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwWindowHint(GLFW_SAMPLES, 16);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);
	last_frame = glfwGetTime();
	
	// Initialize the 3D texture for the grid
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	std::vector<glm::vec3> initial_dots;
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0, grid_resolution / 2.0 - 10, grid_resolution / 2.0));
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0, grid_resolution / 2.0 + 10, grid_resolution / 2.0));
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0 - 10, grid_resolution / 2.0, grid_resolution / 2.0));
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0 + 10, grid_resolution / 2.0, grid_resolution / 2.0));
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0, grid_resolution / 2.0, grid_resolution / 2.0 + 10));
	initial_dots.push_back(glm::vec3(grid_resolution / 2.0, grid_resolution / 2.0, grid_resolution / 2.0 - 10));

	initialize_initial_conditions(initial_dots);
	std::cout << "Initializing boundary conditions from .obj file\n";
	initialize_boundary_conditions("assets/bunny.obj");
	load_initial_conditions_to_texture();	

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

	unsigned int slice_vao, slice_vbo, slice_ebo;
	glGenVertexArrays(1, &slice_vao);
	glBindVertexArray(slice_vao);

	glGenBuffers(1, &slice_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, slice_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &slice_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slice_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;	
	io.Fonts->AddFontFromFileTTF("assets/NotoSans.ttf", 20);
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	ComputeShader compute_shader("shaders/reaction_diffusion.glsl");
	compute_shader.bind();
	compute_shader.set_int("grid_resolution", grid_resolution);
	compute_shader.set_float("time_step", 0.55f);
	compute_shader.set_float("space_step", 1.00f);

	ComputeShader marching_cubes("shaders/marching_cubes.glsl");
	marching_cubes.bind();
	marching_cubes.set_float("grid_resolution", (float)grid_resolution);
	marching_cubes.set_int("grid_tex", 0);

	Shader shader("shaders/default.vert", "shaders/default.frag");
    shader.bind();
	shader.set_int("grid_tex", 0);
	
	// will be bound as both a VBO and SSBO, for generating and rendering vertices
	glGenVertexArrays(1, &grid_vao);
	glBindVertexArray(grid_vao);

	glGenBuffers(1, &grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, 15 * (grid_resolution / 2) * (grid_resolution / 2) * (grid_resolution / 2) * sizeof(MarchingCubeVertex), NULL, GL_DYNAMIC_DRAW);
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

	Shader slice_shader("shaders/slice.vert", "shaders/slice.frag");

	while (!glfwWindowShouldClose(window)) {
		double curr_frame = glfwGetTime();
		double fps = 1.0 / (curr_frame - last_frame);
		if (fps_tracker.size() >= 100) {
			fps_tracker.erase(fps_tracker.begin());
		}
		fps_tracker.push_back(fps);

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

		ImGui::Begin("Reaction Diffusion 3D", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		if (!mouse) ImGui::BeginDisabled();
		std::string fps_tracker_overlay = std::to_string((int)fps) + " FPS";
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::PlotLines("##FPS Meter", fps_tracker.data(), fps_tracker.size(), 0, fps_tracker_overlay.c_str(), 0.0f, 100.0f, ImVec2(0.0f, 80.0f));
		ImGui::PopItemWidth();
		ImGui::SeparatorText("Keyboard Controls");
		ImGui::Text("R - Toggle Mouse Camera Rotation");
		ImGui::Text("Q - (Un)pause Simulation");
		ImGui::Text("Esc - Close Program");

		ImGui::SeparatorText("Simulation");
		ImGui::Checkbox("Paused", &paused);
		ImGui::SameLine();
		if (ImGui::Button("Reset") || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
			glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
		}
		ImGui::SliderFloat("Feed Rate", &a, 0.0f, 0.1f);
		ImGui::SliderFloat("Kill Rate", &b, 0.0f, 0.1f);
		if (ImGui::SliderInt("Resolution ##Grid", &grid_resolution, 10, 128)) {
			mesh_resolution = std::min(mesh_resolution, grid_resolution);
			resize_everything();
		}
		ImGui::SliderInt("Steps/Frame", &dispatches_per_frame, 1, 50);

		ImGui::SeparatorText("Initial Conditions");
		if (ImGui::Button("Central Dot")) {
			std::vector<glm::vec3> dots = {glm::vec3(grid_resolution / 2, grid_resolution / 2, grid_resolution / 2)};
			initialize_initial_conditions(dots);
			load_initial_conditions_to_texture();
		}

		ImGui::SeparatorText("Boundary Conditions");
		if (ImGui::Button("Import .obj")) {
			nfdchar_t *outPath = NULL;		
			nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

			clear_boundary_conditions();
			initialize_boundary_conditions(outPath);
			load_initial_conditions_to_texture();
			
			std::cout << "Selected path " << outPath << "\n";
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			clear_boundary_conditions();
			load_initial_conditions_to_texture();
		}

		ImGui::SeparatorText("Mesh Generation");	
		if (ImGui::SliderInt("Slice", &slice_depth, 0, grid_resolution-1)) slice_depth = std::min(slice_depth, (int)grid_resolution-1);
		ImGui::Image((ImTextureID)slice_texture, ImVec2(gui_width - 20, gui_width - 20));

		ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f);
		ImGui::SliderInt("Resolution ##Mesh", &mesh_resolution, 10, grid_resolution);
		ImGui::Checkbox("Wireframe", &wireframe);
		ImGui::Checkbox("Scalar Field Interpolation", &interpolation);

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);
		if (!mouse) ImGui::EndDisabled();
		ImGui::End();

		compute_shader.bind();
		compute_shader.set_float("Du", Du);
		compute_shader.set_float("Dv", Dv);
		compute_shader.set_float("F", a);
		compute_shader.set_float("k", b);
		compute_shader.set_bool("paused", paused);
		for (int i = 0; i < dispatches_per_frame; i++) {
			glDispatchCompute(grid_resolution / work_group_size, grid_resolution / work_group_size, grid_resolution / work_group_size);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, texture);
		marching_cubes.bind();
		marching_cubes.set_float("threshold", threshold);
		glDispatchCompute((grid_resolution / 2) / work_group_size, (grid_resolution / 2) / work_group_size, (grid_resolution / 2) / work_group_size);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		camera_position = glm::vec3(
			position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
			position.y + radius * glm::sin(glm::radians(pitch)),
			position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
		);

		glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo);
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	

		slice_shader.bind();
		glDisable(GL_CULL_FACE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, texture);
		glViewport(0, 0, grid_resolution, grid_resolution);
		slice_shader.set_int("slice_depth", slice_depth);
		slice_shader.set_int("grid_resolution", grid_resolution);
		slice_shader.set_int("grid_tex", 0);
		glBindVertexArray(slice_vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);	
		glEnable(GL_CULL_FACE);
		glViewport(0, 0, WINDOW_WIDTH - gui_width, WINDOW_HEIGHT);

		shader.bind();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, (float)(WINDOW_WIDTH - gui_width) / (float)WINDOW_HEIGHT, 0.01f, 100.0f);
		shader.set_mat4x4("model", model);
		shader.set_mat4x4("view", view);
		shader.set_mat4x4("proj", proj);
		shader.set_vec3("cam_pos", camera_position);

		glBindVertexArray(grid_vao);
		glDrawArrays(GL_TRIANGLES, 0, 15 * grid_resolution * grid_resolution * grid_resolution);

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
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		mouse = !mouse;
		glfwSetInputMode(window, GLFW_CURSOR, mouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

	if (key == GLFW_KEY_Q && action == GLFW_PRESS) paused = !paused;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
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