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
};

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
