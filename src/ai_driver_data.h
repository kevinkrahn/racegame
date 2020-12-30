#pragma once

#include "math.h"
#include "resource.h"
#include "vehicle_data.h"

struct AIDriverData : public Resource
{
public:
    f32 drivingSkill = 0.5f; // [0,1] how optimal of a path the AI takes on the track
    f32 aggression = 0.5f;   // [0,1] how likely the AI is to go out of its way to attack other drivers
    f32 awareness = 0.5f;    // [0,1] how likely the AI is to attempt to avoid hitting other drivers and obstacles
    f32 fear = 0.5f;         // [0,1] how much the AI tries to evade other drivers
    u32 vehicleIndex = 0;
    VehicleCosmeticConfiguration cosmetics;

    // TODO: add special behaviors that can be selected from a list in the editor

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(drivingSkill);
        s.field(aggression);
        s.field(awareness);
        s.field(fear);
        s.field(vehicleIndex);
        s.field(cosmetics);
    }
};

