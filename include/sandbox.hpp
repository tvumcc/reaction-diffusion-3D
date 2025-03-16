#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Simulator.hpp"
#include "MeshGenerator.hpp"
#include "SliceViewer.hpp"

/**
 * Orchestrates simulation, mesh generation, slice viewing, GUI, and rendering all together.
 * Manages the main loop of the application.
 */
class Sandbox {
public:
    Sandbox(int window_width, int window_height, bool maximized);
    ~Sandbox();

    void run();
private:
    GLFWwindow* window;
    std::unique_ptr<RD3D::Simulator> simulator;
    std::unique_ptr<RD3D::MeshGenerator> mesh_generator;
    std::unique_ptr<RD3D::SliceViewer> slice_viewer;
    OrbitalCamera camera;

    int window_width;
    int window_height;
    int ui_sidebar_width = 350;
    bool mouse = true;

    double last_frame;
    double fps;
    std::vector<float> fps_tracker;

    void initialize_window(int window_width, int window_height);
    void calculate_framerate();
    void init_gui(std::string font_path, int font_size);
    void draw_gui();

    static void resize_callback(GLFWwindow* window, int width, int height);
    static void cursor_pos_callback(GLFWwindow* window, double x, double y);
    static void scroll_callback(GLFWwindow* window, double dx, double dy);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};