//
// Created by ranhe on 10.03.2026.
//

#ifndef VOXELFRUITNINJA_SHADER_H
#define VOXELFRUITNINJA_SHADER_H

#include <glad/gl.h>
#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    void use();
    GLuint getProgram() const;

private:
    GLuint program;
};

#endif //VOXELFRUITNINJA_SHADER_H