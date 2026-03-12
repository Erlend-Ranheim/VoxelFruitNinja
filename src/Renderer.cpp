//
// Created by ranhe on 05.03.2026.
//

#include "Renderer.h"
#include "Config.h"
#include "ModelLoader.h"
#include "Compute.h"


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

    GLuint screenVBO;

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


    glBindImageTexture(
        0,
        outputTexture,
        0,
        GL_FALSE,
        0,
        GL_WRITE_ONLY,
        GL_RGBA32F
        );

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

