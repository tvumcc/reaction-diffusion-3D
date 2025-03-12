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

#define UI_FONT_PATH "assets/NotoSans.ttf"
#define UI_FONT_SIZE 20
#define UI_SIDEBAR_WIDTH 350

Sandbox::Sandbox(int window_width, int window_height) {
    initialize_window(window_width, window_height);
	grid = std::make_unique<Grid>();
	init_gui(UI_FONT_PATH, UI_FONT_SIZE);

	grid_boundary_mesh = std::make_unique<Mesh>("assets/cube.obj");
	boundary_shader = std::make_unique<Shader>("shaders/boundary.vert", "shaders/boundary.frag");
	boundary_offset = glm::vec3(0.0f, 0.0f, 0.0f);
	boundary_scale = 1.0f;

    this->mouse = true;

    this->position = glm::vec3(0.0f, 0.0f, 0.0f);
    this->camera_position = glm::vec3(2.0f, 0.0f, 4.5f);
    this->pitch = 0.0f;
    this->yaw = 180.0f;
    this->radius = 2.0f;
}

Sandbox::~Sandbox() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

/**
 * Initialize the application window and OpenGL using GLFW and GLAD.
 * 
 * @param window_width The initial width of the application window
 * @param window_height The initial height of the application window
 */
void Sandbox::initialize_window(int window_width, int window_height) {
    this->window_width = window_width;
    this->window_height = window_height;
    this->ui_sidebar_width = UI_SIDEBAR_WIDTH;

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
}

/**
 * Initiate the main loop of the application.
 */
void Sandbox::run() {
	while (!glfwWindowShouldClose(window)) {
		calculate_framerate();
		calculate_camera_position();

		render_gui();

		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		grid->set_shader_uniforms();
		set_mesh_uniforms();
		grid->integration_compute_dispatch();
		grid->marching_cubes_compute_dispatch();

		grid->draw_slice();
		glViewport(0, 0, window_width - ui_sidebar_width, window_height);
		grid->mesh_shader.bind();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -0.5f));
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, (float)(window_width - ui_sidebar_width) / (float)window_height, 0.01f, 100.0f);
		grid->mesh_shader.set_mat4x4("model", model);
		grid->mesh_shader.set_mat4x4("view", view);
		grid->mesh_shader.set_mat4x4("proj", proj);
		grid->mesh_shader.set_vec3("cam_pos", camera_position);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDepthMask(GL_TRUE);
		grid->draw_mesh();
		glDepthMask(GL_FALSE);
		draw_boundary_mesh();
		draw_grid_boundary_mesh();
		glDepthMask(GL_TRUE);

		glDisable(GL_BLEND);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

/**
 * Set the uniforms needed by the mesh shader for spatial transformation and lighting.
 */
void Sandbox::set_mesh_uniforms() {
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

/**
 * Calculate the framerate by using the difference in time between the current
 * frame and the previous frame.
 */
void Sandbox::calculate_framerate() {
	double curr_frame = glfwGetTime();
	fps = 1.0 / (curr_frame - last_frame);
	if (fps_tracker.size() >= 100) {
		fps_tracker.erase(fps_tracker.begin());
	}
	fps_tracker.push_back(fps);
	last_frame = curr_frame;
}

/**
 * Calculate the position of the camera in world space using Euler angles to allow
 * for orbit movement around the generated mesh.
 */
void Sandbox::calculate_camera_position() {
	camera_position = glm::vec3(
		position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
		position.y + radius * glm::sin(glm::radians(pitch)),
		position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
	);
}

/**
 * Initialize ImGui and set the font.
 * 
 * @param font_path The path to the .ttf file containing the font to use for the UI
 * @param font_size The desired size of the font specified by font_path
 */
void Sandbox::init_gui(std::string font_path, int font_size) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;	
	io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

/**
 * Render the left UI sidebar using ImGui.
 */
void Sandbox::render_gui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x - ui_sidebar_width, main_viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(ui_sidebar_width, main_viewport->WorkSize.y));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);


	ImGui::Begin("Reaction Diffusion 3D", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | (!mouse ? ImGuiWindowFlags_NoScrollWithMouse : 0));
	if (!mouse) {
		ImGui::BeginDisabled();
	}
	std::string fps_tracker_overlay = std::to_string((int)fps) + " FPS";
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::PlotLines("##FPS Meter", fps_tracker.data(), fps_tracker.size(), 0, fps_tracker_overlay.c_str(), 0.0f, 100.0f, ImVec2(0.0f, 80.0f));
	ImGui::PopItemWidth();

	ImGui::SeparatorText("Keyboard Controls");
	ImGui::Text("R - Toggle Mouse Camera Rotation");
	ImGui::Text("Q - (Un)pause Simulation");
	ImGui::Text("Esc - Close Program");
	if (ImGui::Button("Export .obj")) {
		grid->export_mesh_to_obj();
	}

	ImGui::SeparatorText("Slice Viewer");
	if (ImGui::SliderInt("Slice", &grid->slice_depth, 0, grid->grid_resolution-1)) grid->slice_depth = std::min(grid->slice_depth, (int)grid->grid_resolution-1);
	ImGui::Image((ImTextureID)grid->slice_texture, ImVec2(ui_sidebar_width - 20, ui_sidebar_width - 20));
	if (ImGui::IsItemHovered() && (ImGui::IsMouseDragging(0) || ImGui::IsMouseClicked(0))) {
		ImVec2 mouse_pos = ImGui::GetMousePos();
		ImVec2 image_min = ImGui::GetItemRectMin();
		ImVec2 image_max = ImGui::GetItemRectMax();

		if (mouse_pos.x >= image_min.x && mouse_pos.x < image_max.x &&
			mouse_pos.y >= image_min.y && mouse_pos.y < image_max.y) {
			
			int pixel_x = (int)((mouse_pos.x - image_min.x) / (float)(ui_sidebar_width - 20) * grid->grid_resolution);
			int pixel_y = (int)((mouse_pos.y - image_min.y) / (float)(ui_sidebar_width - 20) * grid->grid_resolution);

			grid->enable_brush(grid->slice_depth, grid->grid_resolution - pixel_y, pixel_x);
		} else grid->disable_brush();
	} else grid->disable_brush();

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

	ImGui::SeparatorText("Boundary Conditions");
	if (ImGui::Button("Import .obj")) {
		nfdchar_t *outPath = NULL;		
		nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

		if (outPath != NULL) {
			boundary_mesh = std::make_unique<Mesh>(outPath);
			boundary_obj_path = outPath;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		grid->clear_boundary_conditions();
		boundary_mesh = nullptr;
		boundary_obj_path = "";
		grid->load_data_to_texture();
	}
	ImGui::SameLine();
	if (ImGui::Button("Voxelize")) {
		if (!boundary_obj_path.empty()) {
			grid->clear_boundary_conditions();
			grid->gen_boundary_conditions(boundary_obj_path, boundary_offset, boundary_scale);
			grid->load_data_to_texture();	
		}
	}
	ImGui::SliderFloat3("Mesh Offset", glm::value_ptr(boundary_offset), -1.0f, 1.0f);
	ImGui::SliderFloat("Mesh Scale", &boundary_scale, 0.0f, 2.0f);

	ImGui::SeparatorText("Mesh Generation");	
	ImGui::SliderFloat("Threshold", &grid->threshold, 0.0f, 1.0f);
	ImGui::SliderInt("Resolution ##Mesh", &grid->mesh_resolution, 10, grid->grid_resolution);
	ImGui::Checkbox("Wireframe", &grid->wireframe);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
	if (!mouse) ImGui::EndDisabled();
	ImGui::End();
}

/**
 * Render the boundary mesh into the world space.
 */
void Sandbox::draw_boundary_mesh() {
	if (boundary_mesh) {
		boundary_shader->bind();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, boundary_offset);
		// model = glm::rotate(model, 3.14159f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(boundary_scale));
		glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(45.0f, (float)(window_width - ui_sidebar_width) / (float)window_height, 0.01f, 100.0f);
		boundary_shader->set_mat4x4("model", model);
		boundary_shader->set_mat4x4("view", view);
		boundary_shader->set_mat4x4("proj", proj);
		boundary_shader->set_vec3("object_color", glm::vec3(1.0f, 0.0f, 0.0f));
		boundary_shader->set_float("transparency", 0.3f);

		boundary_mesh->draw(*boundary_shader, GL_TRIANGLES);
	}
}

/**
 * Render the boundary cube that highlights the boundaries of the 3D grid.
 */
void Sandbox::draw_grid_boundary_mesh() {
	boundary_shader->bind();
	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 view = glm::lookAt(camera_position, position, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 proj = glm::perspective(45.0f, (float)(window_width - ui_sidebar_width) / (float)window_height, 0.01f, 100.0f);
	boundary_shader->set_mat4x4("model", model);
	boundary_shader->set_mat4x4("view", view);
	boundary_shader->set_mat4x4("proj", proj);
	boundary_shader->set_vec3("object_color", glm::vec3(0.0f, 0.0f, 1.0f));
	boundary_shader->set_float("transparency", 0.1f);

	grid_boundary_mesh->draw(*boundary_shader, GL_TRIANGLES);
}

/**
 * Called when the window/framebuffer is resized. Resizes the viewport and updates
 * window_width and window_height.
 * 
 * @param window Handle to the GLFW window
 * @param width New width of the window
 * @param height New height of the window
 */
void Sandbox::resize_callback(GLFWwindow* window, int width, int height) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

    sandbox->window_width = width;
    sandbox->window_height = height;
    glViewport(0, 0, sandbox->window_width - sandbox->ui_sidebar_width, sandbox->window_height);
}

/**
 * Called when the cursor position changes. Updates the camera rotation where
 * differences in the y-coordinate affect pitch and difference in the x-coordinate
 * affect yaw.
 * 
 * @param window Handle to the GLFW window
 * @param x New x-coordinate of the cursor
 * @param y New y-coordinate of the cursor
 */
void Sandbox::cursor_pos_callback(GLFWwindow* window, double x, double y) {
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

/**
 * Called when the mouse wheel is scrolled. Allows for moving the camera closer
 * to or farther away from the mesh.
 * 
 * @param window Handle to the GLFW window
 * @param dx Change in the x-coordinate of the mouse wheel
 * @param dy Change in the y-coordinate of the mouse wheel
 */
void Sandbox::scroll_callback(GLFWwindow* window, double dx, double dy) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	if (sandbox->radius + 0.1 * dy > 0.0f && !sandbox->mouse) sandbox->radius += 0.1 * dy;
}

/**
 * Called when a key is interacted with. Performs miscellanous actions.
 * 
 * @param window Handle to the GLFW window
 * @param key The key that was pressed
 * @param scancode The scancode of the key that was pressed
 * @param action The way that this key was activated
 * @param mods Modifers that were held down during this keypress
 */
void Sandbox::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		sandbox->mouse = !sandbox->mouse;
		glfwSetInputMode(window, GLFW_CURSOR, sandbox->mouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) sandbox->grid->paused = !sandbox->grid->paused;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}