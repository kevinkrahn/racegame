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
    i32 countIndex = -1;

public:
    Start()
    {
        position = glm::vec3(0, 0, 3);
        rotation = glm::rotate(glm::identity<glm::quat>(), glm::vec3(0, 0, PI));
        mesh = g_res.getMesh("world.Start");
        meshLights = g_res.getMesh("world.StartLights");
    }

    void applyDecal(class Decal& decal) override;
    void updateTransform(class Scene* scene) override;
    void onCreate(class Scene* scene) override;
    void onCreateEnd(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
};
