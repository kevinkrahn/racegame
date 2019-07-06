#pragma once

#include "math.h"
#include "datafile.h"

class Entity {
public:
    i32 priority = 0;

    bool isMarkedForDeletion = false;

    virtual ~Entity() {};
    virtual DataFile::Value serialize() { return {}; }
    virtual std::unique_ptr<Entity> deserialize(DataFile::Value& data) { return nullptr; }
    virtual std::unique_ptr<Entity> editorInstantiate(class Editor* editor) { return nullptr; }
    virtual void initializeResources() {}

    virtual void onCreate(class Scene* scene) {}
    virtual void onUpdate(class Renderer* renderer, class Scene* scene, f32 deltaTime) {}
};
