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

    int window_width;
    int window_height;
    int ui_sidebar_width;

    bool mouse;

    glm::vec3 position;
    glm::vec3 camera_position;
    float pitch;
    float yaw;
    float radius;

    double last_frame;
    double fps;
    std::vector<float> fps_tracker;

    Sandbox(int window_width, int window_height);
    ~Sandbox();

    void initialize_window(int window_width, int window_height);
    void run();
    void set_camera_uniforms();

    // GUI Functions
    void init_gui(std::string path, int font_size);
    void render_gui();
    
    // GLFW Callback Functions
private:

};

void resize_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double dx, double dy);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);