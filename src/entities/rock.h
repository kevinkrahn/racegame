#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Rock : public PlaceableEntity
{
    Mesh* mesh;
    Texture* tex;

public:
    Rock(glm::vec3 const& position = {0, 0, 0}, glm::vec3 const& scale = {1, 1, 1},
            f32 zRotation=0.f)
    {
        this->position = position;
        this->scale = scale;
        this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
        mesh = g_resources.getMesh("world.Rock");
        tex = g_resources.getTexture("rock");
    }

    void onCreate(class Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
};
