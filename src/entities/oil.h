#pragma once

#include "../entity.h"
#include "../decal.h"

class Oil : public PlaceableEntity
{
    Decal decal;

public:
    Oil() { setup(glm::vec3(0.f)); }
    Oil(glm::vec3 const& pos)
    {
        setup(pos);
        scale = glm::vec3(3.25f);
    }
    Oil* setup(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void onPreview(RenderWorld* rw) override;
    std::vector<PropPrefabData> generatePrefabProps() override
    {
        return {{ PropCategory::OBSTACLES, "Oil" }};
    }
};
