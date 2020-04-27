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
        scale = glm::vec3(4.f);
    }
    Glue* setup(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void onPreview(RenderWorld* rw) override;
    std::vector<PropPrefabData> generatePrefabProps() override
    {
        return {{ PropCategory::OBSTACLES, "Glue" }};
    }
};
