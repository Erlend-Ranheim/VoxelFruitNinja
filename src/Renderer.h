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
    std::vector<uint8_t> voxels;
    std::vector<float> paletteData;

    int index(int x, int y, int z) const {
        return x + gridSize.x * (y + gridSize.y * z);
    }
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
        glm::vec3 screenToRay(float mouseX, float mouseY);
        void trySlice(glm::vec3, glm::vec3);
private:
        static int const FRUIT_MODELS_AMOUNT = 4;
        static const int MAX_FRUIT_TYPES = 30;
        static constexpr float SLICE_COOLDOWN = 0.08;
        static constexpr float GRAVITY = -9.8f;

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

        GLuint paletteUBO;

        int loadFruitModel(const std::string& path);
        void spawnFruit();
        void sliceFruit(glm::vec3 cutPlane, int hitIndex);
        int uploadNewFruitType(const VoxelModel&, const std::vector<uint8_t>&);
        bool checkFruitHit(glm::vec3, const FruitInstance&, const VoxelModel&, float&);
    bool cpuIntersectAABB(glm::vec3 rayOrigin, glm::vec3 rayDir,glm::vec3 boxMin, glm::vec3 boxMax,float& tEnter, float& tExit);

        float spawnTimer = 0.0f;
        float spawnInterval = 1.5f;
        float sliceCooldown = 0.0f;

        int maxFruits = 12;
};


#endif //VOXELFRUITNINJA_RENDERER_H