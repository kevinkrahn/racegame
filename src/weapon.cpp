#include "weapons/blaster.h"
#include "weapons/explosive_mine.h"

void initializeWeapons()
{
    g_weapons.push_back(std::make_unique<WBlaster>());
    g_weapons.push_back(std::make_unique<WExplosiveMine>());
}
