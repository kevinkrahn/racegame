#pragma once

#include "../entity.h"
#include "../decal.h"
#include "../collision_flags.h"

class StaticDecal : public PlaceableEntity
{
    class Texture* tex;
    Decal decal;
    u32 decalFilter;
    u32 texIndex;

public:
    StaticDecal(u32 texIndex, glm::vec3 const& pos = {0, 0, 0}, u32 decalFilter=DECAL_TRACK);
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onUpdate(class Renderer* renderer, class Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
    const char* getName() const override { return "Decal"; };
    void showDetails() override;
};
