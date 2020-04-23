#pragma once

#include "math.h"
#include "datafile.h"

class Material
{
public:
    i64 guid;
    std::string name;

    void serialize(Serializer& s);
};
