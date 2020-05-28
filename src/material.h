#pragma once

#include "math.h"
#include "datafile.h"
#include "resource.h"
#include "gl.h"

enum struct MaterialType
{
    LIT = 0,
    UNLIT = 1,
};

class Material : public Resource
{
public:
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

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(materialType);

        s.field(isCullingEnabled);
        s.field(castsShadow);
        s.field(isVisible);
        s.field(isDepthReadEnabled);
        s.field(isDepthWriteEnabled);
        s.field(useVertexColors);
        s.field(depthOffset);

        s.field(isTransparent);
        s.field(alphaCutoff);
        s.field(shadowAlphaCutoff);
        s.field(windAmount);

        s.field(color);
        s.field(emit);
        s.field(emitPower);

        s.field(specularPower);
        s.field(specularStrength);
        s.field(specularColor);

        s.field(fresnelScale);
        s.field(fresnelPower);
        s.field(fresnelBias);

        s.field(reflectionStrength);
        s.field(reflectionLod);
        s.field(reflectionBias);

        s.field(colorTexture);
        s.field(normalMapTexture);
    }

    ShaderHandle colorShaderHandle = 0;
    ShaderHandle depthShaderHandle = 0;
    ShaderHandle shadowShaderHandle = 0;
    ShaderHandle pickShaderHandle = 0;
    u32 renderFlags = 0;
    GLuint textureColorHandle = 0;
    GLuint textureNormalHandle = 0;

    void loadShaderHandles(SmallArray<ShaderDefine> additionalDefines={});
};
