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
    Tree(glm::vec3 const& position = {0, 0, 0}, glm::vec3 const& scale = {1, 1, 1}, f32 zRotation=0.f);

    void onCreate(class Scene* scene) override;
    void onRender(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;
    const char* getName() const override { return "Tree"; };
};
