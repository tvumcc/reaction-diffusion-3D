#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#include "shader.hpp"
#include "mesh.hpp"
#include "marching_cubes.hpp"

#include <iostream>

unsigned int WINDOW_WIDTH = 1000;
unsigned int WINDOW_HEIGHT = 800;
float r = (float)WINDOW_WIDTH / WINDOW_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);
void cursor_pos_callback(GLFWwindow*, double x_pos, double y_pos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
float pitch = 0.0f;
float yaw = 90.0f;
float radius = 1.0f;
float threshold = 0.2f;

unsigned int width = 128;
unsigned int height = 128;
unsigned int depth = 128;

int alternate = 0;

float a = 0.035f;
float b = 0.065f;
float Du = 0.08f;
float Dv = 0.04f;
bool paused = true;
bool mouse = true;

int main() {
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

	std::vector<Vertex> mesh_data;
	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vertex vertex = {
			glm::vec3(vertices[i*8], vertices[i*8+1], vertices[i*8+2]),
			glm::vec2(vertices[i*8+3], vertices[i*8+4]),
			glm::vec3(vertices[i*8+5], vertices[i*8+6], vertices[i*8+7])
		};
		mesh_data.push_back(vertex);
	}

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Reaction Diffusion 3D", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	std::cout << "Current Version: " << glGetString(GL_VERSION) << std::endl;
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	if (mouse) 
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);

	// Reaction Diffusion Texture

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	std::vector<glm::vec3> initial_dots;
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0 - 40, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0 + 40, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0 - 40, height / 2.0, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0 + 40, height / 2.0, depth / 2.0));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0, depth / 2.0 + 40));
	initial_dots.push_back(glm::vec3(width / 2.0, height / 2.0, depth / 2.0 - 40));

	// std::vector<std::vector<std::vector<std::vector<float>>>> v(width, std::vector<std::vector<std::vector<float>>>(height, std::vector<std::vector<float>>(depth, std::vector<float>(4, 0))));
	std::vector<float> initial_conditions;

	for (int k = 0; k < depth; k++) {
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				int i1 = (i - 50);
				int j1 = (j - 50);
				int k1 = (k - 50);

				bool is_dot = false;
				for (auto dot : initial_dots) {
					if ((i-(int)dot.x)*(i-(int)dot.x) + (j-(int)dot.y)*(j-(int)dot.y) + (k-(int)dot.z)*(k-(int)dot.z) <= 1*1) {
						is_dot = true;
						break;
					}
				}

				if (is_dot) {
					initial_conditions.push_back(1.0f);
					initial_conditions.push_back(1.0f);
				} else {
					initial_conditions.push_back(1.0f);
					initial_conditions.push_back(0.0f);
				}
				initial_conditions.push_back(0.0f);
				initial_conditions.push_back(0.0f);
			}
		}
	}

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
	glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	// Reaction Diffusion Mesh
	std::vector<Vertex> rd_mesh_vertices;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < depth; k++) {
				Vertex vertex = {
					glm::vec3(i / (float)width, j / (float)height, k / (float)depth),
					glm::vec2(0.0, 0.0),
					glm::vec3(0.0, 0.0, 0.0)
				};
				rd_mesh_vertices.push_back(vertex);
			}
		}
	}
	Mesh rd(rd_mesh_vertices);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	Shader shader("shaders/default.vert", "shaders/default.frag", "shaders/default.geom");
    shader.bind();
	shader.set_int("grid_tex", 0);
	Mesh mesh("assets/icosphere_3.obj");

	ComputeShader compute_shader("shaders/reaction_diffusion.glsl");
	compute_shader.bind();
	compute_shader.set_int("width", width);
	compute_shader.set_int("height", height);
	compute_shader.set_int("depth", depth);
	compute_shader.set_float("F", a);
	compute_shader.set_float("k", b);
	compute_shader.set_float("Du", Du);
	compute_shader.set_float("Dv", Dv);
	compute_shader.set_float("time_step", 0.55f);
	compute_shader.set_float("space_step", 1.00f);

	shader.bind();

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
		process_input(window);
		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui Stuff Goes Here
		ImGui::Begin("Menu");
		ImGui::SliderFloat("Feed Rate", &a, 0.0f, 0.1f);
		ImGui::SliderFloat("Kill Rate", &b, 0.0f, 0.1f);
		ImGui::Text(std::to_string(threshold).c_str());
		ImGui::Checkbox("Paused", &paused);
		if (ImGui::Button("Reset") || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, initial_conditions.data());
			glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
		}
		ImGui::End();

		compute_shader.bind();
		compute_shader.set_float("Du", Du);
		compute_shader.set_float("Dv", Dv);
		compute_shader.set_bool("paused", paused);
		for (int i = 0; i < 20; i++) {
			glDispatchCompute(width / 8, height / 8, depth / 8);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		camera_position = glm::vec3(
			position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
			position.y + radius * glm::sin(glm::radians(pitch)),
			position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
		);


		shader.bind();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.01f, 100.0f);
		shader.set_mat4x4("model", model);
		shader.set_mat4x4("view", view);
		shader.set_mat4x4("proj", proj);
		shader.set_vec3("cam_pos", camera_position);
		shader.set_float("width", (float)width);
		shader.set_float("height", (float)height);
		shader.set_float("depth", (float)depth);
		shader.set_float("threshold", threshold);

		// mesh.draw(shader, GL_TRIANGLES);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, texture);
		rd.draw(shader, GL_POINTS);

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

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		paused = !paused;
	}

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	if (width != 0 && height != 0) {
		WINDOW_WIDTH = width;
		WINDOW_HEIGHT = height;
	}
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