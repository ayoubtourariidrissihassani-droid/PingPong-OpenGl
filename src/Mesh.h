// ---------------------------------------------------------------
// Mesh.h - un maillage : sommets, indices, et les objets OpenGL
// correspondants. Trois attributs sont declares :
//     0 : position           (trois nombres)
//     1 : normale            (trois nombres)
//     2 : coordonnee texture (deux nombres)
//
// Utilisation :
//     std::vector<Vertex> sommets = { ... };
//     std::vector<unsigned int> indices = { ... };
//     Mesh maillage(sommets, indices);
//     maillage.dessiner();
// ---------------------------------------------------------------
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <cstddef>

struct Vertex
{
    glm::vec3 position {0.0f};
    glm::vec3 normale  {0.0f, 1.0f, 0.0f};
    glm::vec2 texture  {0.0f};
};

class Mesh
{
public:
    std::vector<Vertex>       sommets;
    std::vector<unsigned int> indices;

    Mesh(std::vector<Vertex> s, std::vector<unsigned int> i = {})
        : sommets(std::move(s)), indices(std::move(i))
    {
        preparer();
    }

    ~Mesh()
    {
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void dessiner(GLenum mode = GL_TRIANGLES) const
    {
        glBindVertexArray(VAO);
        if (indices.empty())
            glDrawArrays(mode, 0, (GLsizei)sommets.size());
        else
            glDrawElements(mode, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    unsigned int identifiantVAO() const { return VAO; }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    void preparer()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(sommets.size() * sizeof(Vertex)),
                     sommets.data(), GL_STATIC_DRAW);

        if (!indices.empty()) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
                         indices.data(), GL_STATIC_DRAW);
        }

        // attribut 0 : position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, position));
        // attribut 1 : normale
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, normale));
        // attribut 2 : coordonnee de texture
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, texture));

        glBindVertexArray(0);
    }
};
