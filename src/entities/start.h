#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../decal.h"

class Start : public PlaceableEntity
{
    Mesh* mesh;
    Mesh* meshLights;
    Decal finishLineDecal;

public:
    Start()
    {
        mesh = g_resources.getMesh("world.Start");
        meshLights = g_resources.getMesh("world.StartLights");
    }

    void applyDecal(class Decal& decal) override;
    void updateTransform(class Scene* scene) override;
    void onCreateEnd(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
    const char* getName() const override { return "Track Start"; }
};
