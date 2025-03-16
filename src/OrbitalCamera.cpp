#include <glm/gtc/matrix_transform.hpp>

#include "OrbitalCamera.hpp"

OrbitalCamera::OrbitalCamera() {
    update_camera_position();
}

OrbitalCamera::OrbitalCamera(glm::vec3 orbit_position, float aspect_ratio, float radius, float yaw, float pitch) :
    orbit_position(orbit_position),
    aspect_ratio(aspect_ratio),
    radius(radius),
    yaw(yaw),
    pitch(pitch)
{
    update_camera_position();
}

/**
 * Rotate the camera about its orbit point.
 * 
 * @param dx The change in the x position, presumably from the mouse
 * @param dy The change in the y position, presumable from the mouse
 */
void OrbitalCamera::rotate(float dx, float dy) {
    yaw += rotation_sensitivity * dx;

    // Limit vertical orbiting
    float new_pitch = pitch + rotation_sensitivity * dy;
    if (new_pitch + dy < 89.0f && new_pitch + dy > -89.0f) {
        pitch = new_pitch;
    } 

    update_camera_position();
}

/**
 * Change the distance between the camera's position and the orbit point.
 * 
 * @param dr The change in radius
 */
void OrbitalCamera::zoom(float dr) {
    // Limit zoom from passing through the center of orbit
    float new_radius = radius + zoom_sensitivity * dr;
	if (new_radius > 0.0f) {
        radius = new_radius;
    }

    update_camera_position();
}

/**
 * Get the product of the projection and view matrices represented by this camera object.
 */
glm::mat4 OrbitalCamera::get_view_projection_matrix() {
	glm::mat4 view = glm::lookAt(camera_position, orbit_position, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 proj = glm::perspective(fov, aspect_ratio, z_near, z_far);
    return proj * view;
}

/**
 * Set the aspect ratio of the camera for use in projection matrix calculations.
 * 
 * @param aspect_ratio The new aspect ratio of the camera
 */
void OrbitalCamera::set_aspect_ratio(float aspect_ratio) {
    this->aspect_ratio = aspect_ratio;
}

/**
 * Update the camera's position in 3D world space based off of its distance and relative orientation
 * to the orbit point.
 */
void OrbitalCamera::update_camera_position() {
	camera_position = glm::vec3(
		orbit_position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
		orbit_position.y + radius * glm::sin(glm::radians(pitch)),
		orbit_position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
	);
}