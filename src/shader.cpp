#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"

#include <string>
#include <fstream>
#include <iostream>

// Check for either linking for compile errors
void checkErrors(unsigned int ID, ShaderType type, const std::string& filename) {
    int success;
    char error_log[512];

    if (type == ShaderType::Program) {
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, error_log);
            std::cout << "Problem linking " << filename << ":\n" << error_log << std::endl;
        }
    } else {
        std::string type_strs[3] = {"VERTEX", "FRAGMENT", "COMPUTE"};
        std::string type_str = type_strs[(int)type];

        glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(ID, 512, NULL, error_log);
            std::cout << "Problem compiling " << type_str << " shader " << filename << ":\n" << error_log << std::endl;
        }
    }
}

// Create a shader given a file path to a vertex and fragment shader
Shader::Shader(const std::string& vertex_source_path, const std::string& fragment_source_path) {
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
    glAttachShader(ID, FS);
    glLinkProgram(ID);
    checkErrors(ID, ShaderType::Program, vertex_source_path + " and " + fragment_source_path);

    // Delete vertex and fragment shaders as they are already linked to the main shader
    glDeleteShader(VS);
    glDeleteShader(FS);
}

// Create a compute shader given the path to a source file
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

// Bind shader to OpenGL state
void AbstractShader::bind() {
    glUseProgram(this->ID);
}

// Unbind shader from OpenGL state
void AbstractShader::unbind() {
    glUseProgram(0);
}

// Send an integer uniform to the shader
void AbstractShader::set_int(const std::string& identifier, int value) {
    glUniform1i(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

// Send a float uniform to the shader
void AbstractShader::set_float(const std::string& identifier, float value) {
    glUniform1f(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

// Send a boolean uniform to the shader
void AbstractShader::set_bool(const std::string& identifier, bool value) {
    glUniform1i(glGetUniformLocation(this->ID, identifier.c_str()), value);
}

// Send a 4x4 matrix uniform to the shader
void AbstractShader::set_mat4x4(const std::string& identifier, glm::mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

// Send a vec3 uniform to the shader
void AbstractShader::set_vec3(const std::string& identifier, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, glm::value_ptr(value));
}

// Send a vec2 uniform to the shader
void AbstractShader::set_vec2(const std::string& identifier, glm::vec2 value) {
    glUniform2fv(glGetUniformLocation(this->ID, identifier.c_str()), 1, glm::value_ptr(value));
}