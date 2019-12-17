#include "weapon.h"
#include "resources.h"
#include "vehicle.h"

void Weapon::outOfAmmo(Vehicle* vehicle)
{
    if (vehicle->driver->isPlayer)
    {
        g_audio.playSound(&g_res.sounds->nono, SoundType::GAME_SFX);
        // TODO: out of ammo notification?
    }
}

void Weapon::loadSceneData(const char* sceneName)
{
    DataFile::Value::Dict& scene = g_res.getScene(sceneName);
    for (auto& e : scene["entities"].array())
    {
        std::string name = e["name"].string();
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();
        if (name.find("SpawnPoint") != std::string::npos)
        {
            projectileSpawnPoints.push_back(translationOf(transform));
        }
    }
}
