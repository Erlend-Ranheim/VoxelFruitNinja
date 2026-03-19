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
#include "ModelLoader.h"


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

struct VoxelObject {
    glm::vec3 position;
    glm::vec3 scale;
    glm::ivec3 gridSize;
    GLuint voxelTex;
};

class Renderer {
    public:
        Renderer(int width, int height);
        void render();
    private:
        int width, height;

        Camera camera;
        Light light;
        VoxelObject voxelObject;

        Compute raycastCompute;
        Shader screenShader;

        GLuint outputTexture;
        GLuint voxelTexture;

        GLuint screenVAO;
        GLuint screenVBO;

        std::vector<float> paletteData;
};


#endif //VOXELFRUITNINJA_RENDERER_H