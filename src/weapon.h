#pragma once

#include "math.h"
#include <vector>

struct WeaponInfo
{
    const char* name;
    const char* description = "";
    struct Texture* icon;
    i32 price = 0;
    u32 maxUpgradeLevel = 5;

    enum WeaponType
    {
        FRONT_WEAPON,
        REAR_WEAPON,
        SPECIAL_ABILITY,
    } weaponType = FRONT_WEAPON;
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
    glm::mat4 mountTransform;
    glm::vec3 projectileSpawnPoint = glm::vec3(3.f, 0, 0);

    void loadSceneData(const char* sceneName);
    void outOfAmmo(class Vehicle* vehicle);
    virtual void initialize() {}
    virtual ~Weapon() {}
    u32 getMaxAmmo() const { return ammoUnitCount * upgradeLevel; }
    void refillAmmo() { ammo = getMaxAmmo(); }
    virtual void update(class Scene* scene, class Vehicle* vehicle,
            bool fireBegin, bool fireHold, f32 deltaTime) = 0;
    virtual void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            struct VehicleConfiguration const& config, struct VehicleData const& vehicleData) {}
    virtual void reset() {}
};

