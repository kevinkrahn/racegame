#include "driver.h"
#include "resources.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
        i32 controllerID, i32 vehicleIndex, i32 aiIndex)
{
    this->hasCamera = hasCamera;
    this->vehicleIndex = vehicleIndex;
    this->useKeyboard = useKeyboard;
    this->controllerID = controllerID;
    this->isPlayer = isPlayer;

    this->aiIndex = aiIndex;
    if (aiIndex != -1 && !isPlayer)
    {
        this->playerName = g_ais[aiIndex].name;
    }
}

void Driver::aiUpgrades(RandomSeries& series)
{
    assert(aiIndex != -1);
    ComputerDriverData const& ai = g_ais[aiIndex];

//#define AI_DEBUG_PRINT(...) println(__VA_ARGS__)
#define AI_DEBUG_PRINT(...)

    if (vehicleIndex == -1)
    {
        vehicleConfig.color = ai.color;
        vehicleIndex = (i32)ai.vehicleIndex;
        if (ai.wrapIndex != -1)
        {
            vehicleConfig.wrapTextureGuids[0] = g_res.getTexture(g_wrapTextures[ai.wrapIndex])->guid;
            vehicleConfig.wrapColors[0] = ai.wrapColor;
        }
        credits -= g_vehicles[vehicleIndex]->price;
        AI_DEBUG_PRINT("%s bought vehicle %s", playerName.data(), g_vehicles[vehicleIndex]->name);
    }

    // TODO: make AI buy better vehicles over time

    enum UpgradeType
    {
        FRONT_WEAPON1 = 0,
        FRONT_WEAPON2,
        FRONT_WEAPON3,
        REAR_WEAPON1,
        REAR_WEAPON2,
        REAR_WEAPON3,
        SPECIAL_ABILITY,
        ARMOR,
        ENGINE,
        TIRES,
        MISC_PERFORMANCE,
    };
    struct UpgradeChoice
    {
        UpgradeType type;
        f32 priority;
        u32 upgradeLevel;
    };

    const u32 MAX_UPGRADE = 0;

    Array<UpgradeChoice> upgradeChoices = {
        { FRONT_WEAPON1, 20 + ai.aggression, 2 },
        { FRONT_WEAPON1, 18 + ai.aggression * 2.f, 3 },
        { FRONT_WEAPON1, 16 + ai.aggression * 2.f, MAX_UPGRADE },
        { FRONT_WEAPON2, 13 + ai.aggression, 2 },
        { FRONT_WEAPON2, 12 + ai.aggression, 3 },
        { FRONT_WEAPON2, 11 + ai.aggression, MAX_UPGRADE },
        { FRONT_WEAPON3, 8 + ai.aggression, 2 },
        { FRONT_WEAPON3, 7 + ai.aggression, 3 },
        { FRONT_WEAPON3, 6 + ai.aggression, MAX_UPGRADE },
        { REAR_WEAPON1, 13 + ai.aggression + ai.awareness + ai.fear, 2 },
        { REAR_WEAPON1, 9 + ai.aggression + ai.awareness + ai.fear, MAX_UPGRADE },
        { REAR_WEAPON2, 7 + ai.aggression + ai.awareness, 2 },
        { REAR_WEAPON2, 5 + ai.aggression, 3 },
        { REAR_WEAPON2, 4, MAX_UPGRADE },
        { REAR_WEAPON3, 2, 2 },
        { REAR_WEAPON3, 1, 3 },
        { REAR_WEAPON3, 0, MAX_UPGRADE },
        { SPECIAL_ABILITY, 13 + ai.fear, MAX_UPGRADE },
        { ARMOR, 14 + ai.awareness + ai.drivingSkill, 1 },
        { ARMOR, 13 + ai.awareness, 2 },
        { ARMOR, 10 + ai.awareness, 3 },
        { ARMOR, 9  + ai.awareness, 4 },
        { ARMOR, 8  + ai.awareness, 5 },
        { ENGINE, 16 + ai.drivingSkill * 3 + ai.awareness, 1 },
        { ENGINE, 14 + ai.drivingSkill * 2 + ai.awareness, 2 },
        { ENGINE, 12 + ai.drivingSkill, 3 },
        { ENGINE, 11 + ai.drivingSkill, 4 },
        { ENGINE, 10 + ai.drivingSkill, MAX_UPGRADE },
        { TIRES, 15 + ai.drivingSkill * 2 + ai.awareness * 2, 1 },
        { TIRES, 14 + ai.drivingSkill * 2 + ai.awareness, 2 },
        { TIRES, 12 + ai.drivingSkill * 2, 3 },
        { TIRES, 12 + ai.drivingSkill, 4 },
        { TIRES, 11 + ai.drivingSkill, MAX_UPGRADE },
        { MISC_PERFORMANCE, 12 + ai.drivingSkill * 2 + ai.awareness, 1 },
        { MISC_PERFORMANCE, 10 + ai.drivingSkill, MAX_UPGRADE },
    };

    upgradeChoices.sort([&](auto& a, auto& b) { return a.priority > b.priority; });

    Array<i32> frontWeapons;
    Array<i32> rearWeapons;
    Array<i32> specialAbilities;
    for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
    {
        switch (g_weapons[i].info.weaponType)
        {
            case WeaponType::FRONT_WEAPON:
                frontWeapons.push_back(i);
                break;
            case WeaponType::REAR_WEAPON:
                rearWeapons.push_back(i);
                break;
            case WeaponType::SPECIAL_ABILITY:
                specialAbilities.push_back(i);
                break;
        }
    }

    const u32 ENGINE_INDEX = 0;
    const u32 TIRES_INDEX = 1;
    const u32 ARMOR_INDEX = 2;

    bool allDone = false;
    VehicleConfiguration& c = *getVehicleConfig();
    while (!allDone)
    {
        bool boughtSomething = false;
        for (auto& upgradeChoice : upgradeChoices)
        {
            bool restart = false;
            switch(upgradeChoice.type)
            {
                case FRONT_WEAPON1:
                case FRONT_WEAPON2:
                case FRONT_WEAPON3:
                {
                    u32 frontWeaponIndex = upgradeChoice.type - FRONT_WEAPON1;
                    if (getVehicleData()->getWeaponSlotCount(WeaponType::FRONT_WEAPON) < frontWeaponIndex + 1)
                    {
                        continue;
                    }
                    i32 slotIndex = getVehicleData()->getWeaponSlotIndex(WeaponType::FRONT_WEAPON, frontWeaponIndex);
                    i32 index = c.weaponIndices[slotIndex] == -1
                        ? frontWeapons[irandom(series, 0, (u32)frontWeapons.size())]
                        : c.weaponIndices[slotIndex];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.weaponUpgradeLevel[slotIndex] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            AI_DEBUG_PRINT("%s bought front weapon %s %i",
                                    playerName.data(), g_weapons[index].info.name, c.weaponUpgradeLevel[slotIndex] + 1);
                            credits -= g_weapons[index].info.price;
                            c.weaponIndices[slotIndex] = index;
                            ++c.weaponUpgradeLevel[slotIndex];
                            restart = true;
                            boughtSomething = true;
                        }
                    }
                } break;
                case REAR_WEAPON1:
                case REAR_WEAPON2:
                case REAR_WEAPON3:
                {
                    u32 rearWeaponIndex = upgradeChoice.type - REAR_WEAPON1;
                    if (getVehicleData()->getWeaponSlotCount(WeaponType::REAR_WEAPON) < rearWeaponIndex + 1)
                    {
                        continue;
                    }
                    i32 slotIndex = getVehicleData()->getWeaponSlotIndex(WeaponType::REAR_WEAPON, rearWeaponIndex);
                    i32 index = c.weaponIndices[slotIndex] == -1
                        ? rearWeapons[irandom(series, 0, (u32)rearWeapons.size())]
                        : c.weaponIndices[slotIndex];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.weaponUpgradeLevel[slotIndex] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            AI_DEBUG_PRINT("%s bought rear weapon %s %i",
                                    playerName.data(), g_weapons[index].info.name, c.weaponUpgradeLevel[slotIndex] + 1);
                            credits -= g_weapons[index].info.price;
                            c.weaponIndices[slotIndex] = index;
                            ++c.weaponUpgradeLevel[slotIndex];
                            restart = true;
                            boughtSomething = true;
                        }
                    }
                } break;
                case SPECIAL_ABILITY:
                {
                    if (c.specialAbilityIndex == -1)
                    {
                        u32 index = specialAbilities[irandom(series, 0, (u32)specialAbilities.size())];
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            AI_DEBUG_PRINT("%s bought special ability %s", playerName.data(), g_weapons[index].info.name);
                            credits -= g_weapons[index].info.price;
                            c.specialAbilityIndex = index;
                            boughtSomething = true;
                        }
                    }
                } break;
                case ARMOR:
                {
                    auto upgrade = c.getUpgrade(ARMOR_INDEX);
                    if (!upgrade)
                    {
                        break;
                    }
                    u32 upgradeLevel = upgrade ? upgrade->upgradeIndex + 1 : 1;
                    if (upgradeLevel >= upgradeChoice.upgradeLevel)
                    {
                        break;
                    }
                    auto& upgradeInfo = getVehicleData()->availableUpgrades[ARMOR_INDEX];
                    i32 price = upgradeInfo.price * upgradeLevel;
                    if (credits < price)
                    {
                        allDone = true;
                    }
                    else if (c.canAddUpgrade(this, upgradeLevel))
                    {
                        AI_DEBUG_PRINT("%s bought armor %i (%.2f)", upgradeLevel, upgradeChoice.priority);
                        credits -= price;
                        c.addUpgrade(ARMOR_INDEX);
                        restart = true;
                        boughtSomething = true;
                    }
                } break;
                case ENGINE:
                {
                    auto upgrade = c.getUpgrade(ENGINE_INDEX);
                    if (!upgrade)
                    {
                        break;
                    }
                    u32 upgradeLevel = upgrade ? upgrade->upgradeLevel + 1 : 1;
                    if (upgradeLevel >= upgradeChoice.upgradeLevel)
                    {
                        break;
                    }
                    auto& upgradeInfo = getVehicleData()->availableUpgrades[ENGINE_INDEX];
                    i32 price = upgradeInfo.price * upgradeLevel;
                    if (credits < price)
                    {
                        allDone = true;
                    }
                    else if (c.canAddUpgrade(this, ENGINE_INDEX))
                    {
                        AI_DEBUG_PRINT("%s bought engine %i (%.2f)", upgradeLevel, upgradeChoice.priority);
                        credits -= price;
                        c.addUpgrade(ENGINE_INDEX);
                        restart = true;
                        boughtSomething = true;
                    }
                } break;
                case TIRES:
                {
                    auto upgrade = c.getUpgrade(TIRES_INDEX);
                    if (!upgrade)
                    {
                        break;
                    }
                    u32 upgradeLevel = upgrade ? upgrade->upgradeLevel + 1 : 1;
                    if (upgradeLevel >= upgradeChoice.upgradeLevel)
                    {
                        break;
                    }
                    auto& upgradeInfo = getVehicleData()->availableUpgrades[TIRES_INDEX];
                    i32 price = upgradeInfo.price * upgradeLevel;
                    if (credits < price)
                    {
                        allDone = true;
                    }
                    else if (c.canAddUpgrade(this, TIRES_INDEX))
                    {
                        AI_DEBUG_PRINT("%s bought tires %i (%.2f)", upgradeLevel, upgradeChoice.priority);
                        credits -= price;
                        c.addUpgrade(TIRES_INDEX);
                        restart = true;
                        boughtSomething = true;
                    }
                } break;
                case MISC_PERFORMANCE:
                {
                    auto d = getVehicleData();
                    for (u32 i=0; i<(u32)d->availableUpgrades.size(); ++d)
                    {
                        if (i == TIRES_INDEX || i == ENGINE_INDEX || i == ARMOR_INDEX)
                        {
                            continue;
                        }

                        auto upgrade = c.getUpgrade(i);
                        u32 upgradeLevel = upgrade ? upgrade->upgradeLevel + 1 : 1;
                        if (upgradeLevel >= upgradeChoice.upgradeLevel)
                        {
                            continue;
                        }
                        auto& upgradeInfo = getVehicleData()->availableUpgrades[i];
                        i32 price = upgradeInfo.price * upgradeLevel;
                        if (credits < price)
                        {
                            allDone = true;
                            break;
                        }
                        else if (c.canAddUpgrade(this, i))
                        {
                            AI_DEBUG_PRINT("%s bought %s %i", upgradeInfo.name, upgradeLevel);
                            credits -= price;
                            c.addUpgrade(i);
                            restart = true;
                            boughtSomething = true;
                            break;
                        }
                    }
                } break;
            }
            if (restart)
            {
                break;
            }
        }
        if (!boughtSomething)
        {
            break;
        }
    }
}
