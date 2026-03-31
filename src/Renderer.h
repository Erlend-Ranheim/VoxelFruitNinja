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

struct VoxelModel {
    GLuint      voxelTexture;
    glm::ivec3  gridSize;
    std::vector<float> paletteData;
};

struct FruitInstance {
    glm::vec3 position;
    float scale;
    int fruitType;

    glm::vec3 velocity;
    float rotationAngle;
    glm::vec3 rotationAxis;
    float rotationSpeed;
};


class Renderer {
    public:
        Renderer(int width, int height);
        void render();
        void update(float deltaTime);
    private:
        int width, height;

        Camera camera;
        Light light;

        // All loaded fruit types
        std::vector<VoxelModel> fruitModels;

        // All active fruits in the scene
        std::vector<FruitInstance> fruitInstances;

        Compute raycastCompute;
        Shader screenShader;

        GLuint outputTexture;
        GLuint backgroundTexture;

        GLuint screenVAO;
        GLuint screenVBO;

        int loadFruitModel(const std::string& path);
        void spawnFruit();

        float spawnTimer = 0.0f;
        float spawnInterval = 1.5f;

        int maxFruits = 10;
};


#endif //VOXELFRUITNINJA_RENDERER_H