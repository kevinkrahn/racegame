#include "material.h"

void Material::serialize(Serializer& s)
{
    s.field(guid);
    s.field(name);
    s.write("type", ResourceType::MATERIAL);
}
