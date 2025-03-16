#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "Sandbox.hpp"

Sandbox::Sandbox(int window_width, int window_height, bool maximized) {
    initialize_window(window_width, window_height);
	init_gui("assets/NotoSans.ttf", 20);

	simulator = std::make_unique<RD3D::Simulator>();
    mesh_generator = std::make_unique<RD3D::MeshGenerator>(simulator->grid_resolution);
    slice_viewer = std::make_unique<RD3D::SliceViewer>(simulator->grid_resolution);

	if (maximized) glfwMaximizeWindow(window);
}

Sandbox::~Sandbox() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

/**
 * Start the main application loop.
 */
void Sandbox::run() {
	while (!glfwWindowShouldClose(window)) {
		calculate_framerate();

		draw_gui();

		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		simulator->simulate_time_steps();
        mesh_generator->generate(simulator->grid_resolution, simulator->grid_texture);

        // Draw all the meshes to the screen (Reaction Diffusion Mesh, Boundary Mesh, Grid Cube Mesh)
        slice_viewer->render(simulator->grid_resolution, simulator->grid_texture);
		glViewport(0, 0, window_width - ui_sidebar_width, window_height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_TRUE);
        mesh_generator->draw(camera, simulator->grid_resolution);
		glDepthMask(GL_FALSE);
        simulator->boundary.draw_boundary_mesh(camera);
        simulator->boundary.draw_grid_boundary_mesh(camera);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

/**
 * Set up the GLFW window and load GLAD to initialize OpenGL.
 * 
 * @param window_width Initial width of the window 
 * @param window_height Initial height of the window
 */
void Sandbox::initialize_window(int window_width, int window_height) {
    this->window_width = window_width;
    this->window_height = window_height;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(window_width, window_height, "Reaction Diffusion 3D", NULL, NULL);
	glfwMakeContextCurrent(window);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
    glfwSetWindowUserPointer(window, this);
    resize_callback(window, window_width, window_height);

	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
 * Calculate the current framerate of the application based on
 * how long it took for the previous frame to pass.
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
 * Initialize ImGui for later GUI rendering in the main loop.
 * 
 * @param font_path File path to the font to use for the GUI
 * @param font_size Size of the GUI font
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
 * Draw the right sidebar GUI.
 */
void Sandbox::draw_gui(){
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
	if (!mouse) ImGui::BeginDisabled();

	std::string fps_tracker_overlay = std::to_string((int)fps) + " FPS";
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::PlotLines("##FPS Meter", fps_tracker.data(), fps_tracker.size(), 0, fps_tracker_overlay.c_str(), 0.0f, 100.0f, ImVec2(0.0f, 80.0f));
	ImGui::PopItemWidth();

	if (ImGui::CollapsingHeader("Help")) {
		ImGui::SeparatorText("Keyboard Controls");
		ImGui::Text("Q - (Un)pause Simulation");
		ImGui::Text("E - Toggle Mouse Camera Rotation");
		ImGui::Text("R - Reset Mesh");
		ImGui::Text("Esc - Close Program");

		ImGui::SeparatorText("How To Use");
		ImGui::TextWrapped("Try drawing a pattern in the slice viewer by clicking and dragging and then watch it propagate into an interesting looking mesh.");
	}

	ImGui::SeparatorText("Slice Viewer");
	slice_viewer->draw_gui(simulator.get(), simulator->grid_resolution, ui_sidebar_width);

	ImGui::SeparatorText("Simulation");
	simulator->draw_gui(*mesh_generator, *slice_viewer);

	ImGui::SeparatorText("Boundary Conditions");
	simulator->boundary.draw_gui();

	ImGui::SeparatorText("Mesh Generation");	
	mesh_generator->draw_gui(simulator->grid_resolution);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
	if (!mouse) ImGui::EndDisabled();
	ImGui::End();

}

/**
 * GLFW Callbacks
 */

void Sandbox::resize_callback(GLFWwindow* window, int width, int height) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

    sandbox->window_width = width;
    sandbox->window_height = height;
    sandbox->camera.set_aspect_ratio((float)(sandbox->window_width - sandbox->ui_sidebar_width) / sandbox->window_height);
    glViewport(0, 0, sandbox->window_width - sandbox->ui_sidebar_width, sandbox->window_height);
}

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
        sandbox->camera.rotate(dx, dy);
	}
}

void Sandbox::scroll_callback(GLFWwindow* window, double dx, double dy) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

    if (!sandbox->mouse) {
        sandbox->camera.zoom(dy);
		sandbox->camera.rotate(dx, 0);
    }
}

void Sandbox::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Sandbox* sandbox = (Sandbox*)glfwGetWindowUserPointer(window); 

	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		sandbox->mouse = !sandbox->mouse;
		glfwSetInputMode(window, GLFW_CURSOR, sandbox->mouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) sandbox->simulator->toggle_pause();
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_R && action == GLFW_PRESS) sandbox->simulator->reset();
}