#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nfd.h>

#include "sandbox.hpp"

#include <string>
#include <iostream>

Sandbox::Sandbox(int window_width, int window_height) {
    initialize_window(window_width, window_height);
	grid = std::make_unique<Grid>();

    this->mouse = true;

    this->position = glm::vec3(0.0f, 0.0f, 0.0f);
    this->camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
    this->pitch = 0.0f;
    this->yaw = 90.0f;
    this->radius = 1.0f;
}

Sandbox::~Sandbox() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

void Sandbox::initialize_window(int window_width, int window_height) {
    this->window_width = window_width;
    this->window_height = window_height;
    this->ui_sidebar_width = 300;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Initialize GLFW and the window
	window = glfwCreateWindow(window_width, window_height, "Reaction Diffusion 3D", NULL, NULL);
	glfwMakeContextCurrent(window);

    // Initialize GLAD and various callback functions
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
    glfwSetWindowUserPointer(window, this);
    resize_callback(window, window_width, window_height);

	if (mouse) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwWindowHint(GLFW_SAMPLES, 16);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);
	last_frame = glfwGetTime();

	const std::string font_path = "assets/NotoSans.ttf";
	const int font_size = 20;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;	
	io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void Sandbox::run() {
	while (!glfwWindowShouldClose(window)) {
		// Calculate Frame Rate
		double curr_frame = glfwGetTime();
		fps = 1.0 / (curr_frame - last_frame);
		if (fps_tracker.size() >= 100) {
			fps_tracker.erase(fps_tracker.begin());
		}
		fps_tracker.push_back(fps);
		last_frame = curr_frame;

		// Calculate Camera Position for Orbital Movement
		camera_position = glm::vec3(
			position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
			position.y + radius * glm::sin(glm::radians(pitch)),
			position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
		);

		render_gui();


		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		grid->set_shader_uniforms();
		set_camera_uniforms();
		grid->integration_compute_dispatch();
		grid->marching_cubes_compute_dispatch();

		grid->draw_slice();
		glViewport(0, 0, window_width - ui_sidebar_width, window_height);
		grid->draw_mesh();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Sandbox::set_camera_uniforms() {
	grid->mesh_shader.bind();
	glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 proj = glm::perspective(45.0f, (float)(window_width - ui_sidebar_width) / (float)window_height, 0.01f, 100.0f);
	grid->mesh_shader.set_mat4x4("model", model);
	grid->mesh_shader.set_mat4x4("view", view);
	grid->mesh_shader.set_mat4x4("proj", proj);
	grid->mesh_shader.set_vec3("cam_pos", camera_position);
	grid->mesh_shader.set_int("grid_tex", 0);
}

void Sandbox::render_gui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGui Stuff Goes Here
	const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x - ui_sidebar_width, main_viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(ui_sidebar_width, main_viewport->WorkSize.y));
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
	ImGui::Checkbox("Paused", &grid->paused);
	ImGui::SameLine();
	if (ImGui::Button("Reset") || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid->grid_resolution, grid->grid_resolution, grid->grid_resolution, 0, GL_RGBA, GL_FLOAT, grid->data.data());
		glBindImageTexture(0, grid->grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	}
	ImGui::SliderFloat("Feed Rate", &grid->feed_rate, 0.0f, 0.1f);
	ImGui::SliderFloat("Kill Rate", &grid->kill_rate, 0.0f, 0.1f);
	if (ImGui::SliderInt("Resolution ##Grid", &grid->grid_resolution, 10, 128)) {
		grid->mesh_resolution = std::min(grid->mesh_resolution, grid->grid_resolution);
		grid->resize();
	}
	ImGui::SliderInt("Steps/Frame", &grid->integration_dispatches_per_frame, 1, 50);

	ImGui::SeparatorText("Initial Conditions");
	if (ImGui::Button("Central Dot")) {
		std::vector<glm::vec3> dots = {glm::vec3(grid->grid_resolution / 2, grid->grid_resolution / 2, grid->grid_resolution / 2)};
		grid->gen_initial_conditions(dots);
		grid->load_data_to_texture();
	}

	ImGui::SeparatorText("Boundary Conditions");
	if (ImGui::Button("Import .obj")) {
		nfdchar_t *outPath = NULL;		
		nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

		grid->clear_boundary_conditions();
		grid->gen_boundary_conditions(outPath);
		grid->load_data_to_texture();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		grid->clear_boundary_conditions();
		grid->load_data_to_texture();
	}

	ImGui::SeparatorText("Mesh Generation");	
	if (ImGui::SliderInt("Slice", &grid->slice_depth, 0, grid->grid_resolution-1)) grid->slice_depth = std::min(grid->slice_depth, (int)grid->grid_resolution-1);
	ImGui::Image((ImTextureID)grid->slice_texture, ImVec2(ui_sidebar_width - 20, ui_sidebar_width - 20));

	ImGui::SliderFloat("Threshold", &grid->threshold, 0.0f, 1.0f);
	ImGui::SliderInt("Resolution ##Mesh", &grid->mesh_resolution, 10, grid->grid_resolution);
	ImGui::Checkbox("Wireframe", &grid->wireframe);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
	if (!mouse) ImGui::EndDisabled();
	ImGui::End();
}

void resize_callback(GLFWwindow* window, int width, int height) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

    sandbox->window_width = width;
    sandbox->window_height = height;
    glViewport(0, 0, sandbox->window_width - sandbox->ui_sidebar_width, sandbox->window_height);
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	static float last_x = (float)sandbox->window_width / 2.0;
	static float last_y = (float)sandbox->window_height / 2.0;
	static bool first_mouse = true;

	if (first_mouse) {
		last_x = x;
		last_y = y;	
		first_mouse = false;
	}

	float dx = (float)x - last_x;
	float dy = last_y - (float)y;
	last_x = (float)x;
	last_y = (float)y;

	if (!sandbox->mouse) {
		sandbox->yaw += dx;
		if (sandbox->pitch + dy < 89.0f && sandbox->pitch + dy > -89.0f) {
			sandbox->pitch += dy;
		} 
	}

}

void scroll_callback(GLFWwindow* window, double dx, double dy) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	if (sandbox->radius + 0.1 * dy > 0.0f) sandbox->radius += 0.1 * dy;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		sandbox->mouse = !sandbox->mouse;
		glfwSetInputMode(window, GLFW_CURSOR, sandbox->mouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) sandbox->grid->paused = !sandbox->grid->paused;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}