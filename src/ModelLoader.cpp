#include "ModelLoader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    int readInt(std::ifstream& file) {
        int value = 0;
        file.read(reinterpret_cast<char*>(&value), 4);
        return value;
    }

    std::string readChunkId(std::ifstream& file) {
        char id[4];
        file.read(id, 4);
        return std::string(id, 4);
    }

    void loadDefaultPalette(VoxModel& model) {
        // Very simple fallback palette if RGBA chunk is absent.
        // You can replace this later with the official default palette.
        for (int i = 0; i < 256; ++i) {
            model.palette[i] = {255, 255, 255, 255};
        }
    }
}

VoxModel ModelLoader::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open .vox file: " + path);
    }

    std::string magic = readChunkId(file);
    if (magic != "VOX ") {
        throw std::runtime_error("Invalid .vox file: missing VOX header");
    }

    int version = readInt(file);
    std::cout << "VOX version: " << version << std::endl;

    VoxModel model;
    loadDefaultPalette(model);

    bool foundSize = false;
    bool foundXYZI = false;
    bool foundRGBA = false;

    while (file && !file.eof()) {
        if (file.peek() == EOF) {
            break;
        }

        std::string chunkId = readChunkId(file);
        if (chunkId.size() < 4) {
            break;
        }

        int contentSize = readInt(file);
        int childrenSize = readInt(file);

        if (chunkId == "MAIN") {
            continue;
        }
        else if (chunkId == "SIZE") {
            model.sizeX = readInt(file);
            model.sizeY = readInt(file);
            model.sizeZ = readInt(file);

            model.voxels.assign(model.sizeX * model.sizeY * model.sizeZ, 0);
            foundSize = true;
        }
        else if (chunkId == "XYZI") {
            if (!foundSize) {
                throw std::runtime_error("XYZI chunk found before SIZE chunk");
            }

            int numVoxels = readInt(file);

            for (int i = 0; i < numVoxels; ++i) {
                unsigned char x, y, z, colorIndex;
                file.read(reinterpret_cast<char*>(&x), 1);
                file.read(reinterpret_cast<char*>(&y), 1);
                file.read(reinterpret_cast<char*>(&z), 1);
                file.read(reinterpret_cast<char*>(&colorIndex), 1);

                if (x < model.sizeX && y < model.sizeY && z < model.sizeZ) {
                    model.voxels[model.index(x, y, z)] = colorIndex;
                }
            }

            foundXYZI = true;
        }
        else if (chunkId == "RGBA") {
            // palette[0] stays unused, colors go into [1..255]
            for (int i = 1; i <= 255; ++i) {
                VoxColor c;
                file.read(reinterpret_cast<char*>(&c.r), 1);
                file.read(reinterpret_cast<char*>(&c.g), 1);
                file.read(reinterpret_cast<char*>(&c.b), 1);
                file.read(reinterpret_cast<char*>(&c.a), 1);
                model.palette[i] = c;
            }

            VoxColor dummy;
            file.read(reinterpret_cast<char*>(&dummy.r), 1);
            file.read(reinterpret_cast<char*>(&dummy.g), 1);
            file.read(reinterpret_cast<char*>(&dummy.b), 1);
            file.read(reinterpret_cast<char*>(&dummy.a), 1);

            foundRGBA = true;
        }
        else {
            file.seekg(contentSize, std::ios::cur);
        }
    }

    if (!foundSize || !foundXYZI) {
        throw std::runtime_error("Failed to find SIZE and XYZI chunks in .vox file");
    }

    std::cout << "Loaded VOX model: "
              << model.sizeX << " x "
              << model.sizeY << " x "
              << model.sizeZ << std::endl;

    std::cout << "Palette chunk present: " << (foundRGBA ? "yes" : "no") << std::endl;

    return model;
}