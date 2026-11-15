// ---------------------------------------------------------------
// Shader.h - chargement, compilation et edition de liens d'un
// programme de shaders, avec lecture des journaux d'erreurs.
//
// Utilisation :
//     Shader shader("shaders/base.vert", "shaders/base.frag");
//     shader.use();
//     shader.setVec3("uColor", couleur);
// ---------------------------------------------------------------
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID = 0;

    Shader(const std::string& cheminVertex, const std::string& cheminFragment)
    {
        const std::string sourceVertex   = lireFichier(cheminVertex);
        const std::string sourceFragment = lireFichier(cheminFragment);
        if (sourceVertex.empty() || sourceFragment.empty())
            return;

        unsigned int vertex   = compiler(GL_VERTEX_SHADER,   sourceVertex,   cheminVertex);
        unsigned int fragment = compiler(GL_FRAGMENT_SHADER, sourceFragment, cheminFragment);

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        verifier(ID, "PROGRAMME", cheminVertex + " et " + cheminFragment);

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const { glUseProgram(ID); }

    void setBool (const std::string& nom, bool  v) const { glUniform1i(loc(nom), (int)v); }
    void setInt  (const std::string& nom, int   v) const { glUniform1i(loc(nom), v); }
    void setFloat(const std::string& nom, float v) const { glUniform1f(loc(nom), v); }

    void setVec2(const std::string& nom, const glm::vec2& v) const { glUniform2fv(loc(nom), 1, glm::value_ptr(v)); }
    void setVec3(const std::string& nom, const glm::vec3& v) const { glUniform3fv(loc(nom), 1, glm::value_ptr(v)); }
    void setVec4(const std::string& nom, const glm::vec4& v) const { glUniform4fv(loc(nom), 1, glm::value_ptr(v)); }

    void setMat3(const std::string& nom, const glm::mat3& m) const { glUniformMatrix3fv(loc(nom), 1, GL_FALSE, glm::value_ptr(m)); }
    void setMat4(const std::string& nom, const glm::mat4& m) const { glUniformMatrix4fv(loc(nom), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    int loc(const std::string& nom) const
    {
        return glGetUniformLocation(ID, nom.c_str());
    }

    static std::string lireFichier(const std::string& chemin)
    {
        std::ifstream fichier(chemin);
        if (!fichier.is_open()) {
            std::cout << "Shader : fichier introuvable : " << chemin << "\n"
                      << "  Verifiez que le programme est lance depuis le dossier "
                      << "qui contient shaders/ et assets/." << std::endl;
            return {};
        }
        std::stringstream flux;
        flux << fichier.rdbuf();
        return flux.str();
    }

    static unsigned int compiler(GLenum type, const std::string& source, const std::string& chemin)
    {
        unsigned int shader = glCreateShader(type);
        const char* ptr = source.c_str();
        glShaderSource(shader, 1, &ptr, nullptr);
        glCompileShader(shader);
        verifier(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", chemin);
        return shader;
    }

    static void verifier(unsigned int objet, const std::string& etape, const std::string& chemin)
    {
        int ok = 0;
        char journal[1024];
        if (etape == "PROGRAMME") {
            glGetProgramiv(objet, GL_LINK_STATUS, &ok);
            if (!ok) {
                glGetProgramInfoLog(objet, 1024, nullptr, journal);
                std::cout << "Erreur d'edition de liens (" << chemin << ") :\n"
                          << journal << std::endl;
            }
        } else {
            glGetShaderiv(objet, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                glGetShaderInfoLog(objet, 1024, nullptr, journal);
                std::cout << "Erreur de compilation " << etape << " (" << chemin << ") :\n"
                          << journal << std::endl;
            }
        }
    }
};
