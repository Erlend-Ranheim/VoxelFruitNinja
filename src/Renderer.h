//
// Created by ranhe on 05.03.2026.
//

#ifndef VOXELFRUITNINJA_RENDERER_H
#define VOXELFRUITNINJA_RENDERER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Compute.h"
#include "Shader.h"


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
    public:
        Renderer(int width, int height);
        void render();
    private:
        int width, height;
        Camera camera;
        Light light;

        Compute raycastCompute;
        Shader screenShader;

        GLuint computeProgram;
        GLuint outputTexture;

        GLuint screenVAO;
        GLuint screenVBO;

        GLuint screenProgram;
};


#endif //VOXELFRUITNINJA_RENDERER_H