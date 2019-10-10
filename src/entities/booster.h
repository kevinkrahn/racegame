#pragma once

#include "../entity.h"
#include "../decal.h"

class Booster : public PlaceableEntity
{
    struct Texture* tex;
    Decal decal;
    bool backwards = false;
    bool active = false;
    f32 intensity = 1.25f;

public:
    Booster(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onRender(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
    const char* getName() const override { return "Booster"; }
    void showDetails(Scene* scene) override;
};
