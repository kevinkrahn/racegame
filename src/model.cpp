#include "model.h"
#include "resources.h"
#include "game.h"
#include "collision_flags.h"

void Model::serialize(Serializer &s)
{
    s.write("type", ResourceType::MODEL);
    s.field(guid);
    s.field(name);
    s.field(sourceFilePath);
    s.field(sourceSceneName);
    s.field(meshes);
    s.field(objects);
    s.field(modelUsage);
    s.field(density);

    if (s.deserialize)
    {
        for (auto& obj : objects)
        {
            obj.createMousePickCollisionShape(this);
        }
    }
}

void ModelObject::createMousePickCollisionShape(Model* model)
{
    if (mousePickShape)
    {
        mousePickShape->release();
    }
    PxTriangleMesh* collisionGeometry = model->meshes[meshIndex].getCollisionMesh();
    mousePickShape = g_game.physx.physics->createShape(
            PxTriangleMeshGeometry(collisionGeometry, convert(scale)),
            *g_game.physx.defaultMaterial);
    mousePickShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
    mousePickShape->setLocalPose(convert(getTransform()));
}
