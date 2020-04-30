#pragma once

#include "math.h"
#include "datafile.h"

struct RenderItem
{
    u32 vao;
    u32 indexCount;
    glm::mat4 worldTransform;
};

enum struct MaterialType
{
    LIT = 0,
    UNLIT = 1,
};

class Material
{
public:
    i64 guid;
    std::string name;

    MaterialType materialType;

    bool isCullingEnabled = true;
    bool castsShadow = true;
    bool isVisible = true;
    bool isDepthReadEnabled = true;
    bool isDepthWriteEnabled = true;
    bool displayWireframe = false;
    bool useVertexColors = false;
    f32 depthOffset = 0.f;
    f32 windAmount = 0.f;

    bool isTransparent = false;
    f32 contactFadeDistance = 0.f;
    f32 alphaCutoff = 0.f;
    f32 shadowAlphaCutoff = 0.f;

    glm::vec3 color = { 1, 1, 1 };
    glm::vec3 emit = { 0, 0, 0 };
    f32 emitPower = 0.f;

    f32 specularPower = 50.f;
    f32 specularStrength = 0.f;
    glm::vec3 specularColor = { 1, 1, 1 };

    f32 fresnelScale = 0.f;
    f32 fresnelPower = 2.5f;
    f32 fresnelBias = -0.2f;

    f32 reflectionStrength = 0.f;
    f32 reflectionLod = 1.f;
    f32 reflectionBias = 0.2f;

    i64 colorTexture = 0;
    i64 normalMapTexture = 0;

    void serialize(Serializer& s);
    void showPropertiesGui(class Renderer* renderer, class ResourceManager* rm);
};
