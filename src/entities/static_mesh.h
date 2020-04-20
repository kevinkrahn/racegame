#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"

class StaticMesh : public PlaceableEntity
{
    struct MeshItem
    {
        LitRenderable s;
        glm::mat4 transform;
        PxShape* shape;
    };
    SmallVec<MeshItem> meshes;
    u32 meshIndex = 0;

public:
    void applyDecal(class Decal& decal) override;
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
        s.field(meshIndex);
    }
    void updateTransform(class Scene* scene) override;
    u32 getVariationCount() const override;
    void setVariationIndex(u32 variationIndex) override { meshIndex = variationIndex; }
    EditorCategory getEditorCategory(u32 variationIndex) const override;
    void loadMeshes();
};
