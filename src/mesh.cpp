#include "mesh.hpp"

#include <iostream>

/*
    Creates a Mesh object using the passed in vertices directly
*/
Mesh::Mesh(std::vector<Vertex> vertices) {
    this->vertices = vertices;
    init_data();
}

/*
    Creates a Mesh object using data from the Wavefront .obj file 
    specified via path.
*/
Mesh::Mesh(std::string path) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty())
            std::cerr << "[ERROR] TinyObjReader: " << reader.Error();
        exit(1);
    }
    if (!reader.Warning().empty())
        std::cout << "[WARNING] TinyObjReader: " << reader.Warning();

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    
    for (int s = 0; s < shapes.size(); s++) {
        int index_offset = 0;
        for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];
            for (int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                tinyobj::real_t nx = 0.0f;
                tinyobj::real_t ny = 0.0f;
                tinyobj::real_t nz = 0.0f;
                tinyobj::real_t tx = 0.0f;
                tinyobj::real_t ty = 0.0f;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0)
                {
                    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0)
                {
                    tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                Vertex out_vertex = {
                    glm::vec3((float)vx, (float)vy, (float)vz),
                    glm::vec2((float)tx, (float)ty),
                    glm::vec3((float)nx, (float)ny, (float)nz)
                };
                vertices.push_back(out_vertex);
            }

            index_offset += fv;
        }
    }

    init_data();
}

/*
    Draws this mesh using the given Shader object.
    Ensure that all necessary uniforms are bound beforehand.
*/
void Mesh::draw(Shader& shader, GLenum primitive_type) {
    shader.bind(); 
    glBindVertexArray(vertex_array);
    glDrawArrays(primitive_type, 0, vertices.size());
}

/*
    Generates and binds the OpenGL object for this mesh.
*/
void Mesh::init_data() {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
}