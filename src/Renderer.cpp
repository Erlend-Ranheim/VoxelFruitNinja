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


    //Load .VOX model
    VoxModel model = ModelLoader::load("../models/bananversjon1.vox");


    //Passes the voxel model to shader
    glGenTextures(1, &voxelTexture);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);

    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_R8UI,
        model.sizeX,
        model.sizeY,
        model.sizeZ,
        0,
        GL_RED_INTEGER,
        GL_UNSIGNED_BYTE,
        model.voxels.data()
    );

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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


    //Creates instance of one fruit
    voxelObject.position = glm::vec3(0.0f,0.0f,-10.0f);
    voxelObject.scale = glm::vec3(0.4f);
    voxelObject.gridSize = glm::ivec3(model.sizeX, model.sizeY, model.sizeZ);
    voxelObject.voxelTex = voxelTexture;

    // -------------------------
    // Convert palette to float array for GLSL
    // -------------------------
    paletteData.resize(256 * 4);

    for (int i = 0; i < 256; ++i) {
        paletteData[i * 4 + 0] = model.palette[i].r / 255.0f;
        paletteData[i * 4 + 1] = model.palette[i].g / 255.0f;
        paletteData[i * 4 + 2] = model.palette[i].b / 255.0f;
        paletteData[i * 4 + 3] = model.palette[i].a / 255.0f;
    }

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

    glm::vec3 boxSize = glm::vec3(voxelObject.gridSize) * voxelObject.scale;
    glm::vec3 boxMin = voxelObject.position - boxSize * 0.5f;
    glm::vec3 boxMax = voxelObject.position + boxSize * 0.5f;

    glUniform3iv(glGetUniformLocation(program, "gridSize"), 1, &voxelObject.gridSize[0]);
    glUniform3fv(glGetUniformLocation(program, "boxMin"), 1, &boxMin[0]);
    glUniform3fv(glGetUniformLocation(program, "boxMax"), 1, &boxMax[0]);

    // Palette uniform
    glUniform4fv(
        glGetUniformLocation(program, "palette"),
        256,
        paletteData.data()
    );


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, voxelObject.voxelTex);
    glUniform1i(glGetUniformLocation(program, "voxelGrid"), 1);

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

