//
// Created by ranhe on 05.03.2026.
//

#include "Renderer.h"
#include "Config.h"
#include "ModelLoader.h"
#include "Compute.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../libraries/stb_image.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



Renderer::Renderer(int width, int height)
: width(width), height(height), screenShader("../shaders/screen.vert", "../shaders/screen.frag"), raycastCompute("../shaders/raycast.comp")
{
    camera.position = glm::vec3(0.0f,0.0f,0.0f);
    camera.forward = glm::vec3(0.0f,0.0f,-1.0f);
    camera.up = glm::vec3(0.0f,1.0f,0.0f);
    camera.right = glm::vec3(1.0f,0.0f,0.0f);
    camera.fov = 90.0f;
    light.position = glm::vec3(10.0f,10.0f,10.0f);


    //Compute shader
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        width,
        height,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //Screen rendering
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,

        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);

    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);



    // Load fruit types - returns index you use in FruitInstance
    int bananaType = loadFruitModel("../models/banan.vox");
    int tomatType = loadFruitModel("../models/tomat.vox");
    // int appleType = loadFruitModel("../models/apple.vox");  // add more later


    //Load the background image
    glGenTextures(1, &backgroundTexture);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int channels, bgHeight, bgWidth;
    unsigned char *background = stbi_load("../models/FruitNinjaBackground.jpeg", &bgWidth, &bgHeight,  &channels, 0);
    if (background) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bgWidth, bgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, background);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(background);

}

int Renderer::loadFruitModel(const std::string& path) {
    ModelData model = ModelLoader::load(path.c_str());

    VoxelModel fruit;
    fruit.gridSize = glm::ivec3(model.sizeX, model.sizeY, model.sizeZ);

    glGenTextures(1, &fruit.voxelTexture);
    glBindTexture(GL_TEXTURE_3D, fruit.voxelTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8UI,
        model.sizeX, model.sizeY, model.sizeZ,
        0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, model.voxels.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    fruit.paletteData.resize(256 * 4);
    for (int i = 0; i < 256; ++i) {
        fruit.paletteData[i*4+0] = model.palette[i].r / 255.0f;
        fruit.paletteData[i*4+1] = model.palette[i].g / 255.0f;
        fruit.paletteData[i*4+2] = model.palette[i].b / 255.0f;
        fruit.paletteData[i*4+3] = model.palette[i].a / 255.0f;
    }

    fruitModels.push_back(fruit);
    return (int)fruitModels.size() - 1; // returns the index
}

void Renderer::spawnFruit() {
    auto randFloat = [](float min, float max) {
        return min + (float)rand() / RAND_MAX * (max - min);
    };

    if (fruitInstances.size() >= maxFruits)
        return;

    FruitInstance fruit;
    fruit.fruitType     = rand() % fruitModels.size();
    fruit.scale         = randFloat(0.1f, 0.3f);
    fruit.position      = glm::vec3(randFloat(-8.0f, 8.0f), -12.0f, -10.0f);
    fruit.velocity      = glm::vec3(randFloat(-2.0f, 2.0f), randFloat(16.0f, 22.0f), 0.0f);
    fruit.rotationAngle = 0.0f;
    fruit.rotationAxis  = glm::normalize(glm::vec3(randFloat(-1,1), randFloat(-1,1), randFloat(-1,1)));
    fruit.rotationSpeed = randFloat(1.0f, 4.0f);

    fruitInstances.push_back(fruit);
}

void Renderer::render() {

    raycastCompute.use();

    GLuint program = raycastCompute.getProgram();

    glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, &camera.position[0]);
    glUniform3fv(glGetUniformLocation(program, "cameraForward"), 1, &camera.forward[0]);
    glUniform3fv(glGetUniformLocation(program, "cameraRight"), 1, &camera.right[0]);
    glUniform3fv(glGetUniformLocation(program, "cameraUp"), 1, &camera.up[0]);
    glUniform1f(glGetUniformLocation(program, "fov"), camera.fov);
    glUniform2f(glGetUniformLocation(program, "resolution"), (float)width, (float)height);
    glUniform3fv(glGetUniformLocation(program, "lightPosition"), 1, &light.position[0]);


    glUniform1i(glGetUniformLocation(program, "fruitCount"), (int)fruitInstances.size());

    for (int i = 0; i < (int)fruitInstances.size(); i++) {
        const FruitInstance& inst = fruitInstances[i];
        const VoxelModel& model = fruitModels[inst.fruitType];

        std::string base = "fruits[" + std::to_string(i) + "].";

        // Sphere-based AABB
        float radius = glm::length(glm::vec3(model.gridSize) * inst.scale) * 0.5f;
        glm::vec3 boxMin = inst.position - glm::vec3(radius);
        glm::vec3 boxMax = inst.position + glm::vec3(radius);

        glUniform3fv(glGetUniformLocation(program, (base+"boxMin").c_str()),    1, &boxMin[0]);
        glUniform3fv(glGetUniformLocation(program, (base+"boxMax").c_str()),    1, &boxMax[0]);
        glUniform3iv(glGetUniformLocation(program, (base+"gridSize").c_str()),  1, &model.gridSize[0]);
        glUniform1i (glGetUniformLocation(program, (base+"fruitType").c_str()), inst.fruitType);
        glUniform3fv(glGetUniformLocation(program, (base+"center").c_str()),    1, &inst.position[0]);
        glUniform1f (glGetUniformLocation(program, (base+"scale").c_str()),     inst.scale);


        // Rotation matrix — also inside the loop
        glm::mat3 rot = glm::mat3(glm::rotate(
            glm::mat4(1.0f),
            inst.rotationAngle,
            inst.rotationAxis
        ));
        glUniformMatrix3fv(glGetUniformLocation(program, (base+"rotation").c_str()),
                           1, GL_FALSE, glm::value_ptr(rot));
    }

    // Tell the shader which texture unit each fruit type lives on
    for (int t = 0; t < (int)fruitModels.size(); t++) {
        glActiveTexture(GL_TEXTURE3 + t);   // slot 3+t for type t
        glBindTexture(GL_TEXTURE_3D, fruitModels[t].voxelTexture);

        std::string name = "fruitTextures[" + std::to_string(t) + "]";
        glUniform1i(glGetUniformLocation(program, name.c_str()), 3 + t);

        std::string palName = "palettes[" + std::to_string(t) + "]";
        glUniform4fv(glGetUniformLocation(program, palName.c_str()), 256, fruitModels[t].paletteData.data());
    }

    //This is the texture htat is outputted from the compute shader
    glBindImageTexture(
        0,
        outputTexture,
        0,
        GL_FALSE,
        0,
        GL_WRITE_ONLY,
        GL_RGBA32F
        );

    //background image
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glUniform1i(glGetUniformLocation(program, "backgroundTexture"), 2);



    raycastCompute.dispatch((width + 15) / 16, (height + 15) / 16);
    raycastCompute.wait();


    // Drawing the screen quad
    screenShader.use();

    GLuint screenProgram = screenShader.getProgram();
    glUniform1i(glGetUniformLocation(screenProgram, "screenTexture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture);

    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

const float GRAVITY = -9.8f;

void Renderer::update(float deltatime) {
    spawnTimer += deltatime;
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0.0f;
        spawnFruit();
    }
    for (auto& fruit : fruitInstances) {
        //Gravity
        fruit.velocity.y += GRAVITY * deltatime;
        fruit.position += fruit.velocity * deltatime;

        //Spin
        fruit.rotationAngle += fruit.rotationSpeed * deltatime;
    }

    //Removes fruitinstance if out of screen
    fruitInstances.erase(
    std::remove_if(fruitInstances.begin(), fruitInstances.end(),
        [](const FruitInstance& f) {
            return f.position.y < -15.0f; // below screen
        }),
    fruitInstances.end()
);
}

