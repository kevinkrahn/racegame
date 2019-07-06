#pragma once

#include "math.h"
#include "smallvec.h"

struct LightingDefinition
{
    glm::vec3 color = { 1, 1, 1 };
    glm::vec3 emit = { 0, 0, 0 };
    f32 specularPower = 100.f;
    f32 specularStrength = 1.f;
    glm::vec3 specularColor = { 1, 1, 1 };
    f32 fresnelScale = 0.f;
    f32 fresnelPower = 0.f;
    f32 fresnelBias = 0.f;
};

struct Material
{
    u32 shader;
    bool culling = false;
    bool castShadow = true;
    bool depthWrite = true;
    bool depthRead = true;
    f32 depthOffset = 0.f;
    SmallVec<struct Texture*, 4> textures;
    LightingDefinition lighting;
};
