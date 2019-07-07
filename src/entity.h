#pragma once

#include "math.h"
#include "datafile.h"

class Entity {
    bool isMarkedForDeletion = false;

public:
    void destroy() { isMarkedForDeletion = true; }
    bool isDestroyed() { return isMarkedForDeletion; }

    virtual ~Entity() {};
    virtual DataFile::Value serialize() { return {}; }
    virtual std::unique_ptr<Entity> deserialize(DataFile::Value& data) { return nullptr; }
    virtual std::unique_ptr<Entity> editorInstantiate(class Editor* editor) { return nullptr; }

    virtual void onCreate(class Scene* scene) {}
    virtual void onUpdate(class Renderer* renderer, class Scene* scene, f32 deltaTime) {}
};
