#include <imgui/imgui.h>

#include "SliceViewer.hpp"

using namespace RD3D;

SliceViewer::SliceViewer(int grid_resolution) :
    shader("shaders/slice.vert", "shaders/slice.frag")
{
    slice_depth = grid_resolution / 2;

    init_buffers(grid_resolution);
}

/**
 * Render the slice of the 3D texture to an OpenGL framebuffer.
 * 
 * @param grid_resolution The simulation grid's resolution
 * @param grid_texture OpenGL texture object refering to the 3D grid
 */
void SliceViewer::render(int grid_resolution, GLuint grid_texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	

    shader.bind();
    shader.set_int("slice_depth", slice_depth);
    shader.set_int("grid_resolution", grid_resolution);
    shader.set_int("grid_tex", 0);

    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, grid_texture);
    glViewport(0, 0, grid_resolution, grid_resolution);
    glBindVertexArray(slice_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * Resize the 2D slice texture to match the grid resolution.
 * 
 * @param grid_resolution The simulation grid's resolution
 */
void SliceViewer::resize(int grid_resolution) {
	glBindTexture(GL_TEXTURE_2D, slice_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, grid_resolution, grid_resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

/**
 * Draw the GUI section that allows for manipulation of the simulation's mesh generation.
 * 
 * @param simulator Pointer to the Simulator object that this SliceViewer renders from
 * @param grid_resolution The simulation grid's resolution
 * @param ui_sidebar_width Width of the right sidebar GUI to determine how big to draw the slice in the GUI
 */
void SliceViewer::draw_gui(Simulator* simulator, int grid_resolution, int ui_sidebar_width) {
	if (ImGui::SliderInt("Slice", &slice_depth, 0, grid_resolution-1)) slice_depth = std::min(slice_depth, grid_resolution-1);
	ImGui::Image((ImTextureID)slice_texture, ImVec2(ui_sidebar_width - 20, ui_sidebar_width - 20));
	if (ImGui::IsItemHovered() && (ImGui::IsMouseDragging(0) || ImGui::IsMouseClicked(0))) {
		ImVec2 mouse_pos = ImGui::GetMousePos();
		ImVec2 image_min = ImGui::GetItemRectMin();
		ImVec2 image_max = ImGui::GetItemRectMax();

		if (mouse_pos.x >= image_min.x && mouse_pos.x < image_max.x &&
			mouse_pos.y >= image_min.y && mouse_pos.y < image_max.y) {
			
			int pixel_x = (int)((mouse_pos.x - image_min.x) / (float)(ui_sidebar_width - 20) * grid_resolution);
			int pixel_y = (int)((mouse_pos.y - image_min.y) / (float)(ui_sidebar_width - 20) * grid_resolution);

			simulator->enable_brush(slice_depth, grid_resolution - pixel_y, pixel_x);
		} else simulator->disable_brush();
	} else simulator->disable_brush();
}

/**
 * Utility function to set up the vertex buffer, framebuffer, and 2D texture for rendering the slice.
 * 
 * @param grid_resolution The simulation grid's resolution
 */
void SliceViewer::init_buffers(int grid_resolution) {
    float vertices[] = {
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
    };

	// Initialize the FBO for the slice viewer
	glGenFramebuffers(1, &slice_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo);

	// Initialize the texture for the slice viewer
	glGenTextures(1, &slice_texture);
	glBindTexture(GL_TEXTURE_2D, slice_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, grid_resolution, grid_resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, slice_texture, 0);

	glGenVertexArrays(1, &slice_vao);
	glBindVertexArray(slice_vao);

	glGenBuffers(1, &slice_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, slice_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}