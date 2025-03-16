#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"

#include <string>
#include <fstream>
#include <iostream>

/**
 * Check for shader linking or compilation errors.
 * 
 * @param ID ID of the shader in the OpenGL state
 * @param type Type of shader to debug, see ShaderType enum
 * @param filename Source file that the shader was compiled/linked from
 */
void checkErrors(unsigned int ID, ShaderType type, const std::string& filename) {
    int success;
    char error_log[512];

    if (type == ShaderType::Program) { // Check for linking errors
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, error_log);
            std::cout << "Problem linking " << filename << ":\n" << error_log << std::endl;
        }
    } else { // Check for compilation errors
        std::string type_strs[4] = {"VERTEX", "FRAGMENT", "GEOMETRY", "COMPUTE"};
        std::string type_str = type_strs[(int)type];

        glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(ID, 512, NULL, error_log);
            std::cout << "Problem compiling " << type_str << " shader " << filename << ":\n" << error_log << std::endl;
        }
    }
}


/**
 * Create a shader given a file paths to the respective source files.
 * 
 * @param vertex_source_path File path to the Vertex Shader
 * @param fragment_source_path File path to the Fragment Shader
 * @param geometry_source_path File path to the Geometry Shader
 */
Shader::Shader(const std::string& vertex_source_path, const std::string& fragment_source_path, const std::string& geometry_source_path) {
    std::string line, v_text, f_text;
    std::ifstream v_file(vertex_source_path), f_file(fragment_source_path);

    // Read vertex shader from file
    while (std::getline(v_file, line))
        v_text += line + "\n";
    v_file.close();
    const char* v_source = v_text.c_str();

    // Compile vertex shader and check for errors
    unsigned int VS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VS, 1, &v_source, NULL);
    glCompileShader(VS);
    checkErrors(VS, ShaderType::Vertex, vertex_source_path);

    // Read fragment shader from file
    while (std::getline(f_file, line))
        f_text += line + "\n";
    const char* f_source = f_text.c_str();
    f_file.close();

    // Compile fragment shader and check for errors
    unsigned int FS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FS, 1, &f_source, NULL);
    glCompileShader(FS);
    checkErrors(FS, ShaderType::Fragment, fragment_source_path);

    // Link vertex and fragment shaders
    this->ID = glCreateProgram();
    glAttachShader(ID, VS);
    if (geometry_source_path != "NONE") glAttachShader(ID, compile_geometry_shader(geometry_source_path));
    glAttachShader(ID, FS);
    glLinkProgram(ID);
    checkErrors(ID, ShaderType::Program, vertex_source_path + " and " + fragment_source_path);

    // Delete vertex and fragment shaders as they are already linked to the main shader
    glDeleteShader(VS);
    glDeleteShader(FS);
}

/**
 * Utility function to compile a Geometry Shader.
 * 
 * @param geometry_source_path File path to the Geometry Shader
 */
unsigned int Shader::compile_geometry_shader(const std::string& geometry_source_path) {
    std::string line, g_text;
    std::ifstream g_file(geometry_source_path);

    // Read vertex shader from file
    while (std::getline(g_file, line))
        g_text += line + "\n";
    g_file.close();
    const char* g_source = g_text.c_str();

    // Compile vertex shader and check for errors
    unsigned int GS = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(GS, 1, &g_source, NULL);
    glCompileShader(GS);
    checkErrors(GS, ShaderType::Geometry, geometry_source_path);

    return GS;
}

/**
 * Create a compute shader given the path to a source file.
 * 
 * @param compute_source_path File path to the Compute Shader
 */
ComputeShader::ComputeShader(const std::string& compute_source_path) {
    std::string line, text;
    std::ifstream file(compute_source_path);

    // Read compute shader from file
    while(std::getline(file, line))
        text += line + "\n";
    file.close();
    const char* source = text.c_str();

    // Compile compute shader and check for errors
    unsigned int CS = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(CS, 1, &source, NULL);
    glCompileShader(CS);
    checkErrors(CS, ShaderType::Compute, compute_source_path);

    // Link compute shader into a program
    this->ID = glCreateProgram();
    glAttachShader(ID, CS);
    glLinkProgram(ID);
    glDeleteShader(CS);
}

/**
 * Bind shader to OpenGL state.
 */
void AbstractShader::bind() {
    glUseProgram(this->ID);
}

/**
 * Unbind shader from OpenGL state.
 */
void AbstractShader::unbind() {
    glUseProgram(0);
}

/**
 * Send an integer uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_int(const std::string& identifier, int value) {
    glUniform1i(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

/**
 * Send a float uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_float(const std::string& identifier, float value) {
    glUniform1f(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

/**
 * Send a boolean uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_bool(const std::string& identifier, bool value) {
    glUniform1i(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

/**
 * Send a 4x4 matrix uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_mat4x4(const std::string& identifier, glm::mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

/**
 * Send a vec3 uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_vec3(const std::string& identifier, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, glm::value_ptr(value));
}

/**
 * Send a vec3 uniform to the shader.
 * 
 * @param identifier Name of the uniform in the shader source code
 * @param value Value to assign to the shader's uniform
 */
void AbstractShader::set_vec2(const std::string& identifier, glm::vec2 value) {
    glUniform2fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, glm::value_ptr(value));
}