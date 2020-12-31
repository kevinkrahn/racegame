#pragma once

#include "math.h"
#include "datafile.h"

enum struct ResourceType
{
    TEXTURE = 1,
    MODEL = 2,
    SOUND = 3,
    FONT = 4,
    TRACK = 5,
    MATERIAL = 6,
    AI_DRIVER_DATA = 7,
    VINYL_PATTERN = 8,
};

inline u32 mapHash(ResourceType val)
{
    return (u32)val;
}

class Resource
{
public:
    i64 guid;
    Str64 name;
    ResourceType type;

    virtual ~Resource() {}
    virtual void serialize(Serializer& s)
    {
        s.field(type);
        s.field(guid);
        s.field(name);
    }
};
