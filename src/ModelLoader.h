//
// Created by ranhe on 08.03.2026.
//

#ifndef VOXELFRUITNINJA_MODELLOADER_H
#define VOXELFRUITNINJA_MODELLOADER_H


#ifndef VOXELFRUITNINJA_VOXLOADER_H
#define VOXELFRUITNINJA_VOXLOADER_H

#include <array>
#include <string>
#include <vector>

struct VoxColor {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;
};

struct VoxModel {
    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;

    std::vector<unsigned char> voxels;

    // palette[0] unused, palette[1..255] valid
    std::array<VoxColor, 256> palette{};


    int index(int x, int y, int z) const {
        return x + y * sizeX + z * sizeX * sizeY;
    }
};

class ModelLoader {
public:
    static VoxModel load(const std::string& path);
};

#endif


#endif //VOXELFRUITNINJA_MODELLOADER_H