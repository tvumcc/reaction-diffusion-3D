#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "grid.hpp"

#include <vector>
#include <string>
#include <memory>

class Sandbox {
public:
    GLFWwindow* window;
    std::unique_ptr<Grid> grid;

    int window_width; // Width of the application window
    int window_height; // Height of the application window
    int ui_sidebar_width; // Width of the left settings sidebar

    bool mouse; // If true, the mouse cursor can be moved around, if false, it is locked so that it can be used for camera rotation

    // Scene transformation
    glm::vec3 position; // Coordinates of the center of the mesh in world space
    glm::vec3 camera_position; // Coordinates of the camera in world space
    float pitch; // Measures the rotation of the camera up and down
    float yaw; // Measures the rotation of the camera left and right
    float radius; // Distance between the camera and the center of the mesh

    // Frame Rate
    double last_frame; // Time in seconds of the previous frame
    double fps; // The application's current framerate
    std::vector<float> fps_tracker; // Contains the framerates for the last 100 frames

    Sandbox(int window_width, int window_height);
    ~Sandbox();

    void initialize_window(int window_width, int window_height);
    void run();


    // Per frame Utility Functions

    void set_mesh_uniforms();
    void calculate_framerate();
    void calculate_camera_position();


    // GUI Functions

    void init_gui(std::string font_path, int font_size);
    void render_gui();
    

    // GLFW Callback Functions

    static void resize_callback(GLFWwindow* window, int width, int height);
    static void cursor_pos_callback(GLFWwindow* window, double x, double y);
    static void scroll_callback(GLFWwindow* window, double dx, double dy);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
