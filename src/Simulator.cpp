#include <imgui/imgui.h>

#include "simulator.hpp"

using namespace RD3D;

Simulator::Simulator() :
    shader("shaders/reaction_diffusion.glsl"),
    grid(4 * grid_resolution * grid_resolution * grid_resolution, 0.0f)
{
	glGenTextures(1, &grid_texture);
	glBindTexture(GL_TEXTURE_3D, grid_texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    load_data_to_texture();

	boundary.simulator = this;
}

/**
 * Dispatch the compute shader that solves the Gray-Scott Reaction Diffusion PDEs
 */
void Simulator::simulate_time_steps() {
    shader.bind();
	set_shader_uniforms();
    for (int i = 0; i < simulation_time_steps_per_frame; i++) {
        glDispatchCompute(grid_resolution, grid_resolution, grid_resolution);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

/**
 * Reset the simulation. This should preserve boundary values while setting the concentration
 * of chemical V at each grid cell to 0.
 */
void Simulator::reset() {
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, grid.data());
	glBindImageTexture(0, grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
}

/**
 * Resize the grid to match the specified grid_resolution. This will clear the entire grid,
 * including boundary values. 
 */
void Simulator::resize() {
	grid = std::vector<float>(4 * grid_resolution * grid_resolution * grid_resolution, 0.0f);
	boundary.clear_boundary();
    load_data_to_texture();
}

/**
 * Enable the brush which allows the user to set chemical concentrations through the slice viewer.
 * 
 * @param x The x position of the brush
 * @param y The y position of the brush
 * @param z The z position of the brush
 */
void Simulator::enable_brush(int x, int y, int z) {
    brush_x = x;
    brush_y = y;
    brush_z = z;
    brush_enabled = true;
}

/**
 * Disable the brush.
 */
void Simulator::disable_brush() {
    brush_enabled = false;
}

/**
 * Toggle the pause state of the simulation.
 */
void Simulator::toggle_pause() {
	paused = !paused;
}

/**
 * Draw the GUI section that allows for manipulation of the simulation's parameters.
 */
void Simulator::draw_gui(MeshGenerator& mesh_generator, SliceViewer& slice_viewer) {
	ImGui::Checkbox("Paused", &paused);
	ImGui::SameLine();
	if (ImGui::Button("Reset")) reset(); 
	ImGui::SliderFloat("Feed Rate", &feed_rate, 0.0f, 0.1f);
	ImGui::SliderFloat("Kill Rate", &kill_rate, 0.0f, 0.1f);
	if (ImGui::SliderInt("Resolution ##Grid", &grid_resolution, 10, 128)) {
		resize();
		mesh_generator.resize(grid_resolution);
		slice_viewer.resize(grid_resolution);
	}
	ImGui::SliderInt("Steps/Frame", &simulation_time_steps_per_frame, 1, 50);
}

/**
 * Utility function to set all of the simulation's parameters as shader's uniforms.
 */
void Simulator::set_shader_uniforms() {
	shader.bind();
    shader.set_float("F", feed_rate);
    shader.set_float("k", kill_rate);
    shader.set_float("Du", diffusion_u);
    shader.set_float("Dv", diffusion_v);
	shader.set_float("time_step", time_step);
	shader.set_float("space_step", space_step);
	shader.set_bool("paused", paused);
	shader.set_int("brush_x", brush_x);
	shader.set_int("brush_y", brush_y);
	shader.set_int("brush_z", brush_z);
	shader.set_bool("brush_enabled", brush_enabled);
	shader.set_int("grid_resolution", grid_resolution);
}

/**
 * Utility function to load everything from the grid 3D vector to the 3D texture on the GPU.
 */
void Simulator::load_data_to_texture() {
	glBindTexture(GL_TEXTURE_3D, grid_texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, grid_resolution, grid_resolution, grid_resolution, 0, GL_RGBA, GL_FLOAT, grid.data());
	glBindImageTexture(0, grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
}