#pragma once

#include "../entity.h"
#include "../decal.h"
#include "../collision_flags.h"

class StaticDecal : public PlaceableEntity
{
    struct Texture* tex;
    Decal decal;
    u32 decalFilter = DECAL_TRACK;
    i32 texIndex = 0;
    bool beforeMarking = false;

public:
    StaticDecal();
    void onCreateEnd(class Scene* scene) override;
    void updateTransform(class Scene* scene) override;
    void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected) override;
    DataFile::Value serializeState() override;
    void deserializeState(DataFile::Value& data) override;
    void showDetails(Scene* scene) override;
    u32 getVariationCount() const override;
    void setVariationIndex(u32 variationIndex) override;
    EditorCategory getEditorCategory(u32 variationIndex) const override { return EditorCategory::DECALS; }
};
