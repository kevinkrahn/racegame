#include "material.h"
#include "resources.h"

void Material::serialize(Serializer& s)
{
    s.write("type", ResourceType::MATERIAL);

    s.field(guid);
    s.field(name);
    s.field(materialType);

    s.field(isCullingEnabled);
    s.field(castsShadow);
    s.field(isVisible);
    s.field(isDepthReadEnabled);
    s.field(isDepthWriteEnabled);
    s.field(depthOffset);

    s.field(isTransparent);
    s.field(alphaCutoff);

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
