#pragma once
#include <glm/glm.hpp>

/**
 * Represents a camera that rotates in a circle around fixed orbit position.
 */
class OrbitalCamera {
public:
    OrbitalCamera();
    OrbitalCamera(glm::vec3 orbit_position, float aspect_ratio, float radius, float yaw, float pitch);

    void rotate(float dx, float dy);
    void zoom(float dr);

    glm::mat4 get_view_projection_matrix();
    void set_aspect_ratio(float aspect_ratio);
private:
    glm::vec3 orbit_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 camera_position;
    float radius = 2.0f;
    float yaw = 180.0f;
    float pitch = 0.0f;

    float aspect_ratio = 1.0f;
    float fov = 45.0f;
    float z_near = 0.01f;
    float z_far = 100.0f;

    float rotation_sensitivity = 1.0f;
    float zoom_sensitivity = 0.1f;

    void update_camera_position();
};