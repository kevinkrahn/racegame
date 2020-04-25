#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"

class StaticMesh : public PlaceableEntity
{
    i64 modelGuid = 0;
    Model* model = nullptr;

    struct Object
    {
        ModelObject* modelObject = nullptr;
        PxShape* shape = nullptr;
    };

    std::vector<Object> objects;

public:
    void applyDecal(class Decal& decal) override;
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void serializeState(Serializer& s) override;
    void updateTransform(class Scene* scene) override;
    std::vector<PropPrefabData> generatePrefabProps() override;
    const char* getName() const override { return model->name.c_str(); }
};
