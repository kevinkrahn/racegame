#pragma once

#include "../entity.h"
#include "../decal.h"

class Glue : public PlaceableEntity
{
    Decal decal;

public:
    Glue() { setup(glm::vec3(0.f)); }
    Glue(glm::vec3 const& pos)
    {
        setup(pos);
        scale = glm::vec3(3.25f);
    }
    Glue* setup(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override;
    void onPreview(RenderWorld* rw) override;
    Array<PropPrefabData> generatePrefabProps() override
    {
        return {{ PropCategory::OBSTACLES, "Glue" }};
    }
    const char* getName() const override { return "Glue"; }
};
