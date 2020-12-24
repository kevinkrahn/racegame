#pragma once

#include "misc.h"

enum struct WeaponType
{
    FRONT_WEAPON,
    REAR_WEAPON,
    SPECIAL_ABILITY
};

namespace WeaponClass
{
    enum
    {
        HOOD1 = 1 << 0,
        HOOD2 = 1 << 1,
        ROOF1 = 1 << 2,
        ROOF2 = 1 << 3,
        RACK1 = 1 << 4,
        RACK2 = 1 << 5,

        NARROW = 1 << 6
    };
}

struct WeaponSlot
{
    const char* name = "";
    WeaponType weaponType = WeaponType::FRONT_WEAPON;
    u32 weaponClasses = 0;
};

struct WeaponInfo
{
    const char* name;
    const char* description = "";
    struct Texture* icon;
    i32 price = 0;
    u32 maxUpgradeLevel = 5;
    WeaponType weaponType = WeaponType::FRONT_WEAPON;
    u32 weaponClasses = WeaponClass::HOOD1;
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
    Mat4 mountTransform;
    SmallArray<Vec3, 3> projectileSpawnPoints;

    void loadModelData(const char* modelName);
    void outOfAmmo(class Vehicle* vehicle);
    virtual void initialize() {}
    virtual ~Weapon() {}
    u32 getMaxAmmo() const { return ammoUnitCount * upgradeLevel; }
    void refillAmmo() { ammo = getMaxAmmo(); }
    virtual void update(class Scene* scene, class Vehicle* vehicle,
            bool fireBegin, bool fireHold, f32 deltaTime) = 0;
    virtual void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            struct VehicleConfiguration const& config, struct VehicleData const& vehicleData) {}
    virtual void reset() {}
    virtual f32 onDamage(f32 damage, class Vehicle* vehicle) { return damage; }
    virtual bool shouldUse(class Scene* scene, class Vehicle* vehicle) { return false; }
};

