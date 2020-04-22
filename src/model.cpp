#include "model.h"
#include "resources.h"

void Model::serialize(Serializer &s)
{
    s.write("type", ResourceType::MODEL);
    s.field(guid);
    s.field(name);
    s.field(sourceFilePath);
    s.field(sourceSceneName);
    s.field(meshes);
    s.field(objects);
}
