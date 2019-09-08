#pragma once

#include "../entity.h"
#include "../decal.h"

class StaticDecal : public PlaceableEntity
{
    class Texture* tex;
    Decal decal;

public:
    StaticDecal();
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onUpdate(class Renderer* renderer, class Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
};
