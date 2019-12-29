#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Tree : public PlaceableEntity
{
    Mesh* meshTrunk;
    Mesh* meshLeaves;
    Texture* texTrunk;
    Texture* texLeaves;

public:
    Tree();
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    EditorCategory getEditorCategory(u32 variationIndex) const override { return EditorCategory::VEGETATION; }
};
