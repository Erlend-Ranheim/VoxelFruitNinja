//
// Created by ranhe on 10.03.2026.
//

#ifndef VOXELFRUITNINJA_COMPUTE_H
#define VOXELFRUITNINJA_COMPUTE_H
#include "glad/gl.h"
#include <string>

class Compute {
    public:
        Compute(const std::string& path);

        void use();
        void dispatch(int x, int y, int z = 1);
        void wait();

        GLuint getProgram();
    private:
        GLuint program;

};

#endif //VOXELFRUITNINJA_COMPUTE_H