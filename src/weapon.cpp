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
