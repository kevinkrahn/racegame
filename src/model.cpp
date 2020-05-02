#include "model.h"
#include "resources.h"
#include "game.h"
#include "collision_flags.h"

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
