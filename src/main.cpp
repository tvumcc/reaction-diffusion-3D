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

#include <iostream>

unsigned int WINDOW_WIDTH = 1000;
unsigned int WINDOW_HEIGHT = 800;
float r = (float)WINDOW_WIDTH / WINDOW_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);
void cursor_pos_callback(GLFWwindow*, double x_pos, double y_pos);

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
float pitch = 0.0f;
float yaw = 90.0f;
float radius = 2.0f;

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

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL 4.6 Template", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	std::cout << "Current Version: " << glGetString(GL_VERSION) << std::endl;
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	Shader shader("shaders/default.vert", "shaders/default.frag");
    shader.bind();
	Mesh mesh("assets/monkey.obj");

	while (!glfwWindowShouldClose(window)) {
		process_input(window);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui Stuff Goes Here
		ImGui::Begin("Menu");
		ImGui::End();

		camera_position = glm::vec3(
			position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
			position.y + radius * glm::sin(glm::radians(pitch)),
			position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
		);


		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(90.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
		shader.set_mat4x4("model", model);
		shader.set_mat4x4("view", view);
		shader.set_mat4x4("proj", proj);
		shader.set_vec3("cam_pos", camera_position);

		mesh.draw(shader, GL_TRIANGLES);

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
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
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

	yaw += x_offset;	
	if (pitch + y_offset < 89.0f && pitch + y_offset > -89.0f) {
		pitch += y_offset;
	} 
}