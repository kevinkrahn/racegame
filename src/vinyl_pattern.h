#pragma once

#include "math.h"
#include "resource.h"

class VinylPattern : public Resource
{
public:
    i64 colorTextureGuid = 0;
    i64 normalTextureGuid = 0;
    i64 shininessTextureGuid = 0;

    bool canBeUsedByPlayer = true;

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);
        s.field(colorTextureGuid);
        s.field(normalTextureGuid);
        s.field(shininessTextureGuid);
        s.field(canBeUsedByPlayer);
    }
};

