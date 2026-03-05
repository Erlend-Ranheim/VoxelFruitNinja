//
// Created by ranhe on 05.03.2026.
//

#ifndef VOXELFRUITNINJA_RENDERER_H
#define VOXELFRUITNINJA_RENDERER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


struct Camera {
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 right;
    float fov;
};

struct Light {
    glm::vec3 position;
};

class Renderer {
    Renderer();
    Camera camera;
    Light light;


};


#endif //VOXELFRUITNINJA_RENDERER_H