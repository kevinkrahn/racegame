#pragma once

#include "../entity.h"
#include "../decal.h"
#include "../collision_flags.h"

class StaticDecal : public PlaceableEntity
{
    struct Texture* tex;
    Decal decal;
    u32 decalFilter;
    i32 texIndex;
    bool beforeMarking = false;

public:
    StaticDecal() { setPersistent(true); }
    StaticDecal* setup(i32 texIndex, glm::vec3 const& pos = {0, 0, 0}, u32 decalFilter=DECAL_TRACK);
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected) override;
    DataFile::Value serializeState() override;
    void deserializeState(DataFile::Value& data) override;
    const char* getName() const override { return "Decal"; }
    void showDetails(Scene* scene) override;
};
