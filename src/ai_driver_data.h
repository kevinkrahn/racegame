#pragma once

#include "math.h"
#include "resource.h"
#include "vehicle_data.h"

#if 0
enum ActionType
{
    FRONT_WEAPON1 = 0,
    FRONT_WEAPON2,
    FRONT_WEAPON3,
    REAR_WEAPON1,
    REAR_WEAPON2,
    REAR_WEAPON3,
    SPECIAL_ABILITY1,
    SPECIAL_ABILITY2,
    ARMOR,
    ENGINE,
    TIRES,
    SUSPENSION,
    MISC_PERFORMANCE,
    VEHICLE,
};

const char* g_actionNames = {
    "Front Weapon 1",
    "Front Weapon 2",
    "Front Weapon 3",
    "Rear Weapon 1",
    "Rear Weapon 2",
    "Rear Weapon 3",
    "Rear Weapon 3",
    "Special Ability 1",
    "Special Ability 2",
    "Armor",
    "Engine",
    "Tires",
    "Suspension",
    "Misc Performance",
    "Vehicle",
};
#endif

enum struct PurchaseActionType
{
    PERFORMANCE,
    WEAPON,
};

struct PurchaseAction
{
    PurchaseActionType actionType;
    u32 index = 0;
    u32 level = 1;
};

struct AIVehicleConfiguration
{
    i64 vehicleGuid = 0;
    VehicleConfiguration config;
    Array<PurchaseAction> purchaseActions;

    void serialize(Serializer& s)
    {
        s.field(vehicleGuid);
        s.field(config);
    }
};

struct AIDriverData : public Resource
{
public:
    bool usedForChampionshipAI = true;

    f32 drivingSkill = 0.5f; // [0,1] how optimal of a path the AI takes on the track
    f32 aggression = 0.5f;   // [0,1] how likely the AI is to go out of its way to attack other drivers
    f32 awareness = 0.5f;    // [0,1] how likely the AI is to attempt to avoid hitting other drivers and obstacles
    f32 fear = 0.5f;         // [0,1] how much the AI tries to evade other drivers

    Array<AIVehicleConfiguration> vehicles;

    // TODO: add special behaviors that can be selected from a list in the editor

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(usedForChampionshipAI);
        s.field(drivingSkill);
        s.field(aggression);
        s.field(awareness);
        s.field(fear);
        s.field(vehicles);
    }
};

