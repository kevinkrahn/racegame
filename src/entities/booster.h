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
    Booster* setup(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    DataFile::Value serializeState() override
    {
        auto val = PlaceableEntity::serializeState();
        val["backwards"] = DataFile::makeBool(backwards);
        return val;
    }
    void deserializeState(DataFile::Value& data) override
    {
        backwards = data["backwards"].boolean();
        PlaceableEntity::deserializeState(data);
    }
    const char* getName() const override { return "Booster"; }
    void showDetails(Scene* scene) override;
};
