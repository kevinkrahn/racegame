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
    Booster();
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override;
    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
        s.field(backwards);
    }
    void showDetails(Scene* scene) override;
    Array<PropPrefabData> generatePrefabProps() override
    {
        Array<PropPrefabData> a;
        a.push({ PropCategory::OBSTACLES, "Green Booster",
                [](Entity* e) { ((Booster*)e)->backwards = false; } });
        a.push({ PropCategory::OBSTACLES, "Red Booster",
            [](Entity* e) { ((Booster*)e)->backwards = true; } });
        return a;
    }
    const char* getName() const override { return "Booster"; }
};
