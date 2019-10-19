#pragma once

#include "math.h"
#include <vector>

struct WeaponInfo
{
    const char* name;
    const char* description = "";
    const char* icon = nullptr;
    i32 price = 0;
    u32 maxUpgradeLevel = 5;
};

class Weapon
{
public:
    enum FireMode
    {
        ONE_SHOT,
        CONTINUOUS,
    };

    WeaponInfo info;
    FireMode fireMode = FireMode::ONE_SHOT;
    u32 ammo = 0;
    u32 ammoUnitCount = 1;
    u32 upgradeLevel;

    virtual ~Weapon() {}
    u32 getMaxAmmo() const { return ammoUnitCount * upgradeLevel; }
    void refillAmmo() { ammo = getMaxAmmo(); }
    virtual void update(class Scene* scene, class Vehicle* vehicle,
            bool fireBegin, bool fireHold, f32 deltaTime) = 0;
    virtual void render(class Scene* scene, class Vehicle* vehicle,
            glm::mat4 const& mountTransform) const {}
    virtual void reset() {}
};

