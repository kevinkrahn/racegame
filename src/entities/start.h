#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../decal.h"

class Start : public PlaceableEntity
{
    Model* model;
    Decal finishLineDecal;
    i32 countIndex = -1;

public:
    Start()
    {
        position = glm::vec3(0, 0, 3);
        rotation = glm::rotate(glm::identity<glm::quat>(), glm::vec3(0, 0, PI));
        model = g_res.getModel("start");
    }

    void updateTransform(class Scene* scene) override;
    void onCreate(class Scene* scene) override;
    void onCreateEnd(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    void onPreview(RenderWorld* rw) override;
    std::vector<PropPrefabData> generatePrefabProps() override
    {
        return {{ PropCategory::RACE, "Start" }};
    }
};
