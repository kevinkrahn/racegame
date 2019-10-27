#pragma once

#include "../entity.h"
#include "../decal.h"

class Oil : public PlaceableEntity
{
    struct Texture* tex;
    Decal decal;

public:
    Oil() {}
    Oil(glm::vec3 const& pos)
    {
        setup(pos);
        scale = glm::vec3(4.f);
    }
    Oil* setup(glm::vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    const char* getName() const override { return "Oil"; }
};