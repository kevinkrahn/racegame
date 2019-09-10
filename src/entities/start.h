#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../decal.h"

class Start : public PlaceableEntity
{
    Mesh* mesh;
    Decal finishLineDecal;

public:
    Start()
    {
        mesh = g_resources.getMesh("world.Cube.002");
    }

    void applyDecal(class Decal& decal) override;
    void updateTransform(class Scene* scene) override;
    void onCreateEnd(class Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
    const char* getName() const override { return "Track Start"; };
};
