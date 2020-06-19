#include "weapon.h"
#include "resources.h"
#include "vehicle.h"

void Weapon::outOfAmmo(Vehicle* vehicle)
{
    if (vehicle->driver->isPlayer)
    {
        g_audio.playSound(g_res.getSound("nono"), SoundType::GAME_SFX);
        // TODO: out of ammo notification?
    }
}

void Weapon::loadModelData(const char* modelName)
{
    Model* model = g_res.getModel(modelName);
    for (auto const & obj : model->objects)
    {
        if (obj.name.find("SpawnPoint"))
        {
            projectileSpawnPoints.push_back(obj.position);
        }
    }
}
