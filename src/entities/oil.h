#pragma once

#include "../entity.h"
#include "../decal.h"

class Oil : public PlaceableEntity
{
    Decal decal;

public:
    Oil() { setup(Vec3(0.f)); }
    Oil(Vec3 const& pos)
    {
        setup(pos);
        scale = Vec3(3.25f);
    }
    Oil* setup(Vec3 const& pos={0,0,0});
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override;
    void onPreview(RenderWorld* rw) override;
    Array<PropPrefabData> generatePrefabProps() override
    {
        return {{ PropCategory::OBSTACLES, "Oil" }};
    }
    const char* getName() const override { return "Oil"; }
};
