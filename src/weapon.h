#pragma once

#include "math.h"
#include <vector>

class Weapon
{
public:
    const char* name = "Weapon";
    const char* description = "";
    const char* icon = nullptr;
    i32 price = 0;
    u32 maxUpgradeLevel = 5;
    u32 ammoUnitCount = 1;

    virtual ~Weapon() {}
    virtual u32 fire(class Scene* scene, class Vehicle* vehicle) const = 0;
    virtual void render(class Scene* scene, class Vehicle* vehicle,
            glm::mat4 const& mountTransform) const {}
    virtual u32 getAmmoCountForUpgradeLevel(u32 upgradeLevel) const { return upgradeLevel; }
};

std::vector<std::unique_ptr<Weapon>> g_weapons;
void initializeWeapons();
