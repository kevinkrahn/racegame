#pragma once

#include "misc.h"

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
    FireMode fireMode = FireMode::ONE_SHOT;

    WeaponInfo info;
    u32 ammo = 0;
    u32 ammoUnitCount = 1;
    u32 upgradeLevel;
    glm::mat4 mountTransform;
    SmallArray<glm::vec3, 3> projectileSpawnPoints;

    void loadModelData(const char* modelName);
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
    virtual f32 onDamage(f32 damage, class Vehicle* vehicle) { return damage; }
    virtual bool shouldUse(class Scene* scene, class Vehicle* vehicle) { return false; }
};

