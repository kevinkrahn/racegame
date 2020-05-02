#pragma once

#include "resource.h"

class TrackData : public Resource
{
public:
    DataFile::Value data;

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);
        if (s.deserialize)
        {
            data = DataFile::makeDict(s.dict);
        }
        else
        {
            // TODO: save
        }
    }
};
