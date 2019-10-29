#include "driver.h"
#include "resources.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
        u32 controllerID, i32 vehicleIndex, i32 colorIndex, i32 aiIndex)
{
    this->hasCamera = hasCamera;
    this->vehicleIndex = vehicleIndex;
    this->useKeyboard = useKeyboard;
    this->controllerID = controllerID;
    this->isPlayer = isPlayer;

    if (vehicleIndex != -1)
    {
        VehicleConfiguration vehicleConfig;
        vehicleConfig.colorIndex = colorIndex;
        ownedVehicles.push_back({ vehicleIndex, vehicleConfig });
    }

    if (aiIndex != -1 && !isPlayer)
    {
        this->playerName = g_ais[aiIndex].name;
        this->aiIndex = aiIndex;
    }
}

void Driver::aiUpgrades(RandomSeries& series)
{
    assert(aiIndex != -1);

    if (ownedVehicles.empty())
    {
        std::vector<u32> affordableVehicles;
        for (u32 i=0; i<(u32)g_vehicles.size(); ++i)
        {
            if (credits >= g_vehicles[i]->price)
            {
                affordableVehicles.push_back(i);
            }
        }
        vehicleIndex = affordableVehicles[
            irandom(series, 0, (u32)affordableVehicles.size())];
        VehicleConfiguration vehicleConfig;
        vehicleConfig.colorIndex = irandom(series, 0, ARRAY_SIZE(g_vehicleColors));
        ownedVehicles.push_back({ vehicleIndex, vehicleConfig });
        credits -= g_vehicles[vehicleIndex]->price;
        print(playerName, " bought vehicle: ", g_vehicles[vehicleIndex]->name, '\n');
    }

    enum UpgradeType
    {
        FRONT_WEAPON1,
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

    ComputerDriverData ai = g_ais[aiIndex];
    std::vector<UpgradeChoice> upgradeChoices = {
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

    std::sort(upgradeChoices.begin(), upgradeChoices.end(),
            [&](auto& a, auto& b) { return a.priority > b.priority; });

    std::vector<i32> frontWeapons;
    std::vector<i32> rearWeapons;
    std::vector<i32> specialAbilities;
    for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
    {
        switch (g_weapons[i].info.weaponType)
        {
            case WeaponInfo::FRONT_WEAPON:
                frontWeapons.push_back(i);
                break;
            case WeaponInfo::REAR_WEAPON:
                rearWeapons.push_back(i);
                break;
            case WeaponInfo::SPECIAL_ABILITY:
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
        for (auto& upgradeChoice : upgradeChoices)
        {
            bool restart = false;
            switch(upgradeChoice.type)
            {
                case FRONT_WEAPON1:
                {
                    if (getVehicleData()->frontWeaponCount < 1)
                    {
                        continue;
                    }
                    i32 index = c.frontWeaponIndices[0] == -1
                        ? frontWeapons[irandom(series, 0, (u32)frontWeapons.size())]
                        : c.frontWeaponIndices[0];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.frontWeaponUpgradeLevel[0] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought front weapon ", g_weapons[index].info.name, ' ',
                                    c.frontWeaponUpgradeLevel[0] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.frontWeaponIndices[0] = index;
                            ++c.frontWeaponUpgradeLevel[0];
                            restart = true;
                        }
                    }
                } break;
                case FRONT_WEAPON2:
                {
                    if (getVehicleData()->frontWeaponCount < 2)
                    {
                        continue;
                    }
                    i32 index = c.frontWeaponIndices[1] == -1
                        ? frontWeapons[irandom(series, 0, (u32)frontWeapons.size())]
                        : c.frontWeaponIndices[1];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.frontWeaponUpgradeLevel[1] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought front weapon ", g_weapons[index].info.name, ' ',
                                    c.frontWeaponUpgradeLevel[1] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.frontWeaponIndices[1] = index;
                            ++c.frontWeaponUpgradeLevel[1];
                            restart = true;
                        }
                    }
                } break;
                case FRONT_WEAPON3:
                {
                    if (getVehicleData()->frontWeaponCount < 3)
                    {
                        continue;
                    }
                    i32 index = c.frontWeaponIndices[2] == -1
                        ? frontWeapons[irandom(series, 0, (u32)frontWeapons.size())]
                        : c.frontWeaponIndices[2];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.frontWeaponUpgradeLevel[2] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought front weapon ", g_weapons[index].info.name, ' ',
                                    c.frontWeaponUpgradeLevel[2] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.frontWeaponIndices[2] = index;
                            ++c.frontWeaponUpgradeLevel[2];
                            restart = true;
                        }
                    }
                } break;
                case REAR_WEAPON1:
                {
                    if (getVehicleData()->rearWeaponCount < 1)
                    {
                        continue;
                    }
                    i32 index = c.rearWeaponIndices[0] == -1
                        ? rearWeapons[irandom(series, 0, (u32)rearWeapons.size())]
                        : c.rearWeaponIndices[0];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.rearWeaponUpgradeLevel[0] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought rear weapon ", g_weapons[index].info.name, ' ',
                                    c.rearWeaponUpgradeLevel[0] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.rearWeaponIndices[0] = index;
                            ++c.rearWeaponUpgradeLevel[0];
                            restart = true;
                        }
                    }
                } break;
                case REAR_WEAPON2:
                {
                    if (getVehicleData()->rearWeaponCount < 2)
                    {
                        continue;
                    }
                    i32 index = c.rearWeaponIndices[1] == -1
                        ? rearWeapons[irandom(series, 0, (u32)rearWeapons.size())]
                        : c.rearWeaponIndices[1];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.rearWeaponUpgradeLevel[1] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought rear weapon ", g_weapons[index].info.name, ' ',
                                    c.rearWeaponIndices[1] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.rearWeaponIndices[1] = index;
                            ++c.rearWeaponUpgradeLevel[1];
                            restart = true;
                        }
                    }
                } break;
                case REAR_WEAPON3:
                {
                    if (getVehicleData()->rearWeaponCount < 3)
                    {
                        continue;
                    }
                    i32 index = c.rearWeaponIndices[2] == -1
                        ? rearWeapons[irandom(series, 0, (u32)rearWeapons.size())]
                        : c.rearWeaponIndices[2];
                    u32 maxLevel = upgradeChoice.upgradeLevel > 0
                        ? upgradeChoice.upgradeLevel : g_weapons[index].info.maxUpgradeLevel;
                    if (c.rearWeaponUpgradeLevel[2] < maxLevel)
                    {
                        if (g_weapons[index].info.price > credits)
                        {
                            allDone = true;
                        }
                        else
                        {
                            print(playerName, " bought rear weapon ", g_weapons[index].info.name, ' ',
                                    c.rearWeaponUpgradeLevel[2] + 1, '\n');
                            credits -= g_weapons[index].info.price;
                            c.rearWeaponIndices[2] = index;
                            ++c.rearWeaponUpgradeLevel[2];
                            restart = true;
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
                            print(playerName, " bought special ability ", g_weapons[index].info.name, '\n');
                            credits -= g_weapons[index].info.price;
                            c.specialAbilityIndex = index;
                        }
                    }
                } break;
                case ARMOR:
                {
                    auto upgrade = c.getUpgrade(ARMOR_INDEX);
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
                        print(playerName, " bought armor ", upgradeLevel,
                                '(', upgradeChoice.priority, ")\n");
                        credits -= price;
                        c.addUpgrade(ARMOR_INDEX);
                        restart = true;
                    }
                } break;
                case ENGINE:
                {
                    auto upgrade = c.getUpgrade(ENGINE_INDEX);
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
                        print(playerName, " bought engine ", upgradeLevel,
                                '(', upgradeChoice.priority, ")\n");
                        credits -= price;
                        c.addUpgrade(ENGINE_INDEX);
                        restart = true;
                    }
                } break;
                case TIRES:
                {
                    auto upgrade = c.getUpgrade(TIRES_INDEX);
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
                        print(playerName, " bought tires ", upgradeLevel,
                                '(', upgradeChoice.priority, ")\n");
                        credits -= price;
                        c.addUpgrade(TIRES_INDEX);
                        restart = true;
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
                            print(playerName, " bought ", upgradeInfo.name, " ",
                                    upgradeLevel, '\n');
                            credits -= price;
                            c.addUpgrade(i);
                            restart = true;
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
    }
}
