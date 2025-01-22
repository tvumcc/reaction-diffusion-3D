#pragma once
#include <glm/glm.hpp>

#include <string>

enum class ShaderType {
    Vertex = 0,
    Fragment,
    Compute,
    Program
};

class AbstractShader {
public:
    unsigned int ID;
    /*Uniform setting methods*/

    void set_int(const std::string& variable_name, int value);
    void set_float(const std::string& variable_name, float value);
    void set_bool(const std::string& varibale_name, bool value);
    void set_mat4x4(const std::string& variable_name, glm::mat4 value);
    void set_vec3(const std::string& variable_name, glm::vec3 value);
    void set_vec2(const std::string& variable_name, glm::vec2 value);

    /*Binding methods*/

    void bind();
    void unbind();
};

class Shader : public AbstractShader {
public:
    Shader(const std::string& vertex_source_path, const std::string& fragment_source_path);
};

class ComputeShader : public AbstractShader {
public:
    ComputeShader(const std::string& source_path);
};