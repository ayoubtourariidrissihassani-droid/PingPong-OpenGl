// ---------------------------------------------------------------
// Camera.h - camera libre pilotee au clavier et a la souris.
// Toutes les vitesses sont exprimees par seconde : multipliez
// toujours par deltaTime.
//
// Utilisation :
//     Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
//     ...
//     camera.clavier(Camera::AVANT, deltaTime);
//     camera.souris(dx, dy);
//     camera.molette(offset);
//     glm::mat4 view = camera.matriceVue();
// ---------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    enum Direction { AVANT, ARRIERE, GAUCHE, DROITE, HAUT, BAS };

    glm::vec3 position;
    glm::vec3 avant  {0.0f, 0.0f, -1.0f};
    glm::vec3 haut   {0.0f, 1.0f,  0.0f};
    glm::vec3 droite {1.0f, 0.0f,  0.0f};
    glm::vec3 hautMonde {0.0f, 1.0f, 0.0f};

    float lacet   = -90.0f;   // rotation horizontale, en degres
    float tangage =   0.0f;   // rotation verticale, en degres

    float vitesse      = 2.5f;   // unites par seconde
    float sensibilite  = 0.1f;   // degres par pixel
    float champDeVision = 45.0f; // angle d'ouverture, en degres

    explicit Camera(glm::vec3 depart = glm::vec3(0.0f, 0.0f, 3.0f))
        : position(depart)
    {
        recalculer();
    }

    glm::mat4 matriceVue() const
    {
        return glm::lookAt(position, position + avant, haut);
    }

    glm::mat4 matriceProjection(float rapportLargeurHauteur,
                                float proche = 0.1f, float lointain = 100.0f) const
    {
        return glm::perspective(glm::radians(champDeVision),
                                rapportLargeurHauteur, proche, lointain);
    }

    void clavier(Direction direction, float deltaTime)
    {
        const float pas = vitesse * deltaTime;
        if (direction == AVANT)   position += avant  * pas;
        if (direction == ARRIERE) position -= avant  * pas;
        if (direction == GAUCHE)  position -= droite * pas;
        if (direction == DROITE)  position += droite * pas;
        if (direction == HAUT)    position += hautMonde * pas;
        if (direction == BAS)     position -= hautMonde * pas;
    }

    void souris(float deplacementX, float deplacementY, bool bornerTangage = true)
    {
        lacet   += deplacementX * sensibilite;
        tangage += deplacementY * sensibilite;

        if (bornerTangage)
            tangage = glm::clamp(tangage, -89.0f, 89.0f);

        recalculer();
    }

    void molette(float deplacement)
    {
        champDeVision = glm::clamp(champDeVision - deplacement, 20.0f, 75.0f);
    }

private:
    void recalculer()
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(lacet)) * cos(glm::radians(tangage));
        direction.y = sin(glm::radians(tangage));
        direction.z = sin(glm::radians(lacet)) * cos(glm::radians(tangage));

        avant  = glm::normalize(direction);
        droite = glm::normalize(glm::cross(avant, hautMonde));
        haut   = glm::normalize(glm::cross(droite, avant));
    }
};
