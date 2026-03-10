//
// Created by ranhe on 10.03.2026.
//

#include "Compute.h"
#include <fstream>
#include <sstream>
#include <iostream>

Compute::Compute(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();
    const char* src = source.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glDeleteShader(shader);
}

void Compute::use() {
    glUseProgram(program);
}

void Compute::dispatch(int x, int y, int z) {
    glDispatchCompute(x, y, z);
}

void Compute::wait() {
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

GLuint Compute::getProgram() {
    return program;
}