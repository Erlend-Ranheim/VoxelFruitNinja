
#include "Renderer.h"
#include "Config.h"
#include "ModelLoader.h"
#include "Compute.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../libraries/stb_image.h"
#include <set>

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


    //SSBO
    glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, paletteUBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_FRUIT_TYPES * 256 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, paletteUBO);

    // Load fruit types
    int bananaType = loadFruitModel("../models/banan.vox");
    int tomatType = loadFruitModel("../models/tomat.vox");
    int melonType = loadFruitModel("../models/vannmelon.vox");
    int appelsinType = loadFruitModel("../models/appelsin.vox");


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
        std::cout << "Failed to load background" << std::endl;
    }

    stbi_image_free(background);

}

int Renderer::loadFruitModel(const std::string& path) {
    ModelData model = ModelLoader::load(path.c_str());

    VoxelModel fruit;
    fruit.gridSize = glm::ivec3(model.sizeX, model.sizeY, model.sizeZ);
    fruit.voxels = model.voxels;

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

//small function to get random numbers
auto randFloat = [](float min, float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
};

//Function to spawn fruit with random traits
void Renderer::spawnFruit() {
    if (fruitInstances.size() >= maxFruits)
        return;

    FruitInstance fruit;
    fruit.fruitType     = rand() % FRUIT_MODELS_AMOUNT;
    fruit.scale         = randFloat(0.1f, 0.15f);
    fruit.position      = glm::vec3(randFloat(-8.0f, 8.0f), -12.5f, -12.0f);
    fruit.velocity      = glm::vec3(randFloat(-2.0f, 2.0f), randFloat(16.0f, 20.0f), 0.0f);
    fruit.rotationAngle = 0.0f;
    fruit.rotationAxis  = glm::normalize(glm::vec3(randFloat(-1,1), randFloat(-1,1), randFloat(-1,1)));
    fruit.rotationSpeed = randFloat(1.0f, 4.0f);

    fruitInstances.push_back(fruit);
}

int Renderer::uploadNewFruitType(const VoxelModel& source, const std::vector<uint8_t>& voxels) {

    // Look for a free slot in memory
    int slot = -1;
    for (int t = FRUIT_MODELS_AMOUNT; t < (int)fruitModels.size(); t++) {
        if (fruitModels[t].voxelTexture == 0) {
            slot = t;
            break;
        }
    }

    VoxelModel newModel;
    newModel.gridSize    = source.gridSize;
    newModel.paletteData = source.paletteData;
    newModel.voxels      = voxels;



    glGenTextures(1, &newModel.voxelTexture);
    glBindTexture(GL_TEXTURE_3D, newModel.voxelTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8UI,
        newModel.gridSize.x, newModel.gridSize.y, newModel.gridSize.z,
        0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, voxels.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (slot != -1) {
        fruitModels[slot] = newModel;
        return slot;
    }
    if ((int)fruitModels.size() >= MAX_FRUIT_TYPES) {
        std::cout << "max fruit types exceeded!\n";
        glDeleteTextures(1, &newModel.voxelTexture);
        return 0;
    }

    fruitModels.push_back(newModel);
    return (int)fruitModels.size() - 1;
}

void Renderer::sliceFruit(glm::vec3 cutPlane, int fruitIndex) {
    FruitInstance original = fruitInstances[fruitIndex];
    int originalFruitType = original.fruitType;

    glm::ivec3 gridSize = fruitModels[originalFruitType].gridSize;
    glm::vec3 halfSize  = glm::vec3(gridSize) * original.scale * 0.5f;
    glm::vec3 voxelSize = (halfSize * 2.0f) / glm::vec3(gridSize);

    std::vector<uint8_t> voxelsA(fruitModels[originalFruitType].voxels.size(), 0);
    std::vector<uint8_t> voxelsB(fruitModels[originalFruitType].voxels.size(), 0);

    glm::mat3 rot = glm::mat3(glm::rotate(glm::mat4(1.0f),
                              original.rotationAngle, original.rotationAxis));

    for (int z = 0; z < gridSize.z; z++)
    for (int y = 0; y < gridSize.y; y++)
    for (int x = 0; x < gridSize.x; x++) {
        uint8_t val = fruitModels[originalFruitType].voxels[fruitModels[originalFruitType].index(x, y, z)];
        if (val == 0) continue;

        glm::vec3 localPos = -halfSize + (glm::vec3(x,y,z) + 0.5f)  * voxelSize;
        glm::vec3 worldPos = original.position + rot * localPos;

        float side = glm::dot(cutPlane, worldPos-original.position);

        if (side >= 0)
            voxelsA[fruitModels[originalFruitType].index(x, y, z)] = val;
        else
            voxelsB[fruitModels[originalFruitType].index(x, y, z)] = val;
    }

    //Check if fruit actually was sliced
    int countA = 0, countB = 0;
    for (auto v : voxelsA) if (v > 0) countA++;
    for (auto v : voxelsB) if (v > 0) countB++;

    // If one half is empty, the cut missed, don't slice
    if (countA == 0 || countB == 0)
        return;

    // Upload two new fruit types
    int typeA = uploadNewFruitType(fruitModels[originalFruitType], voxelsA);
    int typeB = uploadNewFruitType(fruitModels[originalFruitType], voxelsB);


    FruitInstance halfA = original;
    halfA.fruitType     = typeA;
    halfA.velocity     += glm::vec3(randFloat(-3.0f, -0.5f), randFloat(0.5f, 3.0f), 0.0f);
    halfA.rotationSpeed = randFloat(3.0f, 6.0f);

    FruitInstance halfB = original;
    halfB.fruitType     = typeB;
    halfB.velocity     += glm::vec3(randFloat(0.5f, 3.0f), randFloat(0.5f, 3.0f), 0.0f);
    halfB.rotationSpeed = randFloat(3.0f, 6.0f);

    // Remove original, add halves
    fruitInstances.erase(fruitInstances.begin() + fruitIndex);
    fruitInstances.push_back(halfA);
    fruitInstances.push_back(halfB);
}

bool Renderer::cpuIntersectAABB(glm::vec3 rayOrigin, glm::vec3 rayDir,glm::vec3 boxMin, glm::vec3 boxMax,float& tEnter, float& tExit) {
    glm::vec3 invDir = 1.0f / rayDir;
    glm::vec3 t0 = (boxMin - rayOrigin) * invDir;
    glm::vec3 t1 = (boxMax - rayOrigin) * invDir;
    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);
    tEnter = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    tExit  = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
    return tExit >= glm::max(tEnter, 0.0f);
}

bool Renderer::checkFruitHit(glm::vec3 rayDir, const FruitInstance& inst, const VoxelModel& model, float& outT) {
    // Sphere AABB check first
    float radius = glm::length(glm::vec3(model.gridSize) * inst.scale) * 0.5f;
    glm::vec3 boxMin = inst.position - glm::vec3(radius);
    glm::vec3 boxMax = inst.position + glm::vec3(radius);

    float tEnter, tExit;
    if (!cpuIntersectAABB(camera.position, rayDir, boxMin, boxMax, tEnter, tExit))
        return false;

    // Transform ray to local space
    glm::mat3 rot    = glm::mat3(glm::rotate(glm::mat4(1.0f), inst.rotationAngle, inst.rotationAxis));
    glm::mat3 invRot = glm::transpose(rot);

    glm::vec3 localOrigin = invRot * (camera.position - inst.position);
    glm::vec3 localDir    = invRot * rayDir;

    glm::vec3 halfSize    = glm::vec3(model.gridSize) * inst.scale * 0.5f;
    glm::vec3 localBoxMin = -halfSize;
    glm::vec3 localBoxMax =  halfSize;

    if (!cpuIntersectAABB(localOrigin, localDir, localBoxMin, localBoxMax, tEnter, tExit))
        return false;

    outT = tEnter;
    // Step through voxels
    glm::vec3 voxelSize = (localBoxMax - localBoxMin) / glm::vec3(model.gridSize);
    glm::vec3 startPos  = localOrigin + localDir * glm::max(tEnter, 0.0f);
    glm::vec3 gridPosF  = (startPos - localBoxMin) / voxelSize;

    glm::ivec3 voxel = glm::clamp(glm::ivec3(glm::floor(gridPosF)), glm::ivec3(0), model.gridSize - 1);
    glm::ivec3 stepDir(glm::sign(localDir));
    glm::vec3  tDelta = glm::abs(voxelSize / localDir);

    glm::vec3 voxelMin    = localBoxMin + glm::vec3(voxel) * voxelSize;
    glm::vec3 voxelMax    = voxelMin + voxelSize;
    glm::vec3 nextBoundary(
        localDir.x > 0 ? voxelMax.x : voxelMin.x,
        localDir.y > 0 ? voxelMax.y : voxelMin.y,
        localDir.z > 0 ? voxelMax.z : voxelMin.z
    );

    glm::vec3 tMaxV(
        localDir.x != 0 ? (nextBoundary.x - startPos.x) / localDir.x : 1e30f,
        localDir.y != 0 ? (nextBoundary.y - startPos.y) / localDir.y : 1e30f,
        localDir.z != 0 ? (nextBoundary.z - startPos.z) / localDir.z : 1e30f
    );

    //DDA traversal
    //96 checks is the worst case for 32x32x32 grids since diagonal is 32+32+32 = 96
    for (int i = 0; i < 96; i++) {
        if (glm::any(glm::lessThan(voxel, glm::ivec3(0))) ||
            glm::any(glm::greaterThanEqual(voxel, model.gridSize)))
            break;

        // Check CPU voxel data
        uint8_t val = model.voxels[model.index(voxel.x, voxel.y, voxel.z)];
        if (val > 0)
            return true;

        //choses next step
        if (tMaxV.x < tMaxV.y && tMaxV.x < tMaxV.z) {
            voxel.x += stepDir.x; tMaxV.x += tDelta.x;
        } else if (tMaxV.y < tMaxV.z) {
            voxel.y += stepDir.y; tMaxV.y += tDelta.y;
        } else {
            voxel.z += stepDir.z; tMaxV.z += tDelta.z;
        }
    }

    return false;
}



void Renderer::trySlice(glm::vec3 rayDir1, glm::vec3 rayDir2) {
    //early opt out if slice has happened recently
    if (sliceCooldown > 0.0f)
        return;
    //find cut plane
    glm::vec3 cutPlane = cross(rayDir2, rayDir1);



    int   hitIndex = -1;
    float closestDistance = 1e30f;

    //loop thorugh list
    for (int i = 0; i < (int)fruitInstances.size(); i++) {
        float hitDistance;
        if (checkFruitHit(rayDir1, fruitInstances[i], fruitModels[fruitInstances[i].fruitType], hitDistance)) {
            if (hitDistance < closestDistance) {
                closestDistance = hitDistance;
                hitIndex = i;
            }
        }
    }
    if (hitIndex != -1) {
        sliceFruit(cutPlane, hitIndex);
        sliceCooldown = SLICE_COOLDOWN;
    }
}

glm::vec3 Renderer::screenToRay(float mouseX, float mouseY) {
    float aspectRatio = (float)width / (float)height;
    glm::vec2 uv = (glm::vec2(mouseX, height - mouseY) / glm::vec2(width, height)) * 2.0f - 1.0f;
    uv.x *= aspectRatio;
    float f = glm::tan(glm::radians(camera.fov * 0.5f));
    return glm::normalize(
        camera.forward + uv.x * f * camera.right + uv.y * f * camera.up
    );
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


        // Rotation matrix
        glm::mat3 rot = glm::mat3(glm::rotate(
            glm::mat4(1.0f),
            inst.rotationAngle,
            inst.rotationAxis
        ));
        glUniformMatrix3fv(glGetUniformLocation(program, (base+"rotation").c_str()),
                           1, GL_FALSE, glm::value_ptr(rot));
    }

    // Tell the shader which texture unit each fruit type lives on
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, paletteUBO);
    for (int t = 0; t < (int)fruitModels.size(); t++) {
        glActiveTexture(GL_TEXTURE3 + t);   // slot 3+t for type t
        glBindTexture(GL_TEXTURE_3D, fruitModels[t].voxelTexture);

        std::string name = "fruitTextures[" + std::to_string(t) + "]";
        glUniform1i(glGetUniformLocation(program, name.c_str()), 3 + t);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER,
        t * 256 * sizeof(glm::vec4),
        256 * sizeof(glm::vec4),
        fruitModels[t].paletteData.data());
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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


void Renderer::update(float deltatime) {
    spawnTimer += deltatime;

    if (sliceCooldown > 0.0f)
        sliceCooldown -= deltatime;

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

    // clean up the cut fruit
    std::set<int> usedTypes;
    for (auto& inst : fruitInstances)
        usedTypes.insert(inst.fruitType);

    // Deletes GPU textures
    for (int t = 4; t < (int)fruitModels.size(); t++) {
        if (usedTypes.find(t) == usedTypes.end()) {
            glDeleteTextures(1, &fruitModels[t].voxelTexture);
            fruitModels[t].voxelTexture = 0;
            fruitModels[t].voxels.clear();
            fruitModels[t].paletteData.clear();
        }
    }
}

