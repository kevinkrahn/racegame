#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Rock : public PlaceableEntity
{
    Mesh* mesh;
    Texture* tex;

public:
    Rock()
    {
        mesh = g_resources.getMesh("world.Rock");
        tex = g_resources.getTexture("rock");
    }

    void onCreate(class Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
};
