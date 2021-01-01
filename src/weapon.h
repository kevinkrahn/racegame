#pragma once

#include "misc.h"
#include "datafile.h"

enum struct WeaponType
{
    FRONT_WEAPON,
    REAR_WEAPON,
    SPECIAL_ABILITY,
};

// TODO: remove
const Str16 g_tags[] = {
    "hood-1",
    "hood-2",
    "roof-1",
    "roof-2",
    "rack-1",
    "rack-2",
    "narrow",
};

struct WeaponInfo
{
    const char* name;
    const char* description = "";
    struct Texture* icon;
    i32 price = 0;
    u32 maxUpgradeLevel = 5;
    WeaponType weaponType = WeaponType::FRONT_WEAPON;
    Str16 tags[4];
};

struct TagGroup
{
    Str16 tags[3];

    void serialize(Serializer& s)
    {
        s.field(tags);
    }
};

TagGroup tags(const char* s) { return TagGroup{ { s } }; }
TagGroup tags(const char* s1, const char* s2) { return TagGroup{ { s1, s2 } }; }
TagGroup tags(const char* s1, const char* s2, const char* s3) { return TagGroup{ { s1, s2, s3 } }; }

struct WeaponSlot
{
    Str32 name = "";
    WeaponType weaponType = WeaponType::FRONT_WEAPON;
    TagGroup tagGroups[3];

    void serialize(Serializer& s)
    {
        s.field(weaponType);
        s.field(tagGroups);
    }

    bool matchesWeapon(WeaponInfo const& w) const
    {
        for (u32 i=0; i<ARRAY_SIZE(tagGroups); ++i)
        {
            bool allMatch = true;
            for (u32 j=0; j<ARRAY_SIZE(tagGroups[i].tags); ++j)
            {
                for (u32 k=0; k<ARRAY_SIZE(w.tags); ++k)
                {
                    if (tagGroups[i].tags[j] != w.tags[k])
                    {
                        allMatch = false;
                    }
                }
            }
            if (allMatch)
            {
                return true;
            }
        }
        return false;
    }
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

