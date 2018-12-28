#include "scene.h"
#include "game.h"
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(const char* name)
{
    auto& sceneData = game.resources.getScene(name);
    for (auto& e : sceneData["entities"].array())
    {
        if (e["type"].string() == "MESH")
        {
            staticEntities.push_back({
                game.resources.getMesh(e["data_name"].string().c_str()).renderHandle,
                glm::make_mat4((f32*)e["matrix"].bytearray().data())
            });
        }
    }
}

void Scene::onStart()
{
}

void Scene::onUpdate(f32 deltaTime)
{
    game.renderer.setBackgroundColor(glm::vec3(0.1, 0.1, 0.1));
    game.renderer.setViewportCount(1);
    game.renderer.setViewportCamera(0, { 5, 5, 5 }, { 0, 0, 0 }, 50.f);
    for (auto const& e : staticEntities)
    {
        game.renderer.drawMesh(e.renderHandle, e.worldTransform);
    }
}

void Scene::onEnd()
{
}

