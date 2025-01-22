cmake_minimum_required(VERSION 3.23)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(template)

add_subdirectory("lib/glfw")
find_package(OpenGL REQUIRED)
include_directories(
	${OPENGL_INCLUDE_DIRS} 
	${CMAKE_CURRENT_SOURCE_DIR}/include 
	${CMAKE_CURRENT_SOURCE_DIR}/lib 
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include 
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glm
	${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(${CMAKE_PROJECT_NAME}
	${SRC_FILES}

	# GLAD
	lib/glad/src/glad.c
	lib/stb/stb_image.cpp

	# ImGui
	lib/imgui/imgui.cpp
	lib/imgui/imgui_demo.cpp
	lib/imgui/imgui_draw.cpp
	lib/imgui/imgui_tables.cpp
	lib/imgui/imgui_widgets.cpp
	lib/imgui/backends/imgui_impl_glfw.cpp
	lib/imgui/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${OPENGL_LIBRARIES} glfw)
