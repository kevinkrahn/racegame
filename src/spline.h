#pragma once

#include "entity.h"
#include "renderable.h"
#include "material.h"
#include "resources.h"

struct SplinePoint
{
    glm::vec3 position;
    glm::vec3 handleOffsetA;
    glm::vec3 handleOffsetB;

    void serialize(Serializer& s)
    {
        s.field(position);
        s.field(handleOffsetA);
        s.field(handleOffsetB);
    }
};

class Spline : public Entity
{
    friend class SplineMode;

    // serialized
    Array<SplinePoint> points;
    i64 modelGuid = 0;
    f32 scale = 1.f;

    struct MeshInfo
    {
        Mesh mesh;
        Material* material = nullptr;
        Mesh collisionMesh;
        PxShape* collisionShape = nullptr;
        PxShape* collisionShapeForSelection = nullptr;

        MeshInfo() {}
        MeshInfo(MeshInfo&& other) = default;

        ~MeshInfo()
        {
            // TODO: find out why this crashes
            //mesh.destroy();
            //collisionMesh.destroy();
        }
    };
    Array<MeshInfo> meshes;
    bool isDirty = true;
    PxRigidStatic* actor = nullptr;
    ActorUserData physicsUserData;

public:
    ~Spline()
    {
        if (actor)
        {
            actor->release();
        }
    }

    void serializeState(Serializer& s) override
    {
        s.field(points);
        s.field(scale);
        s.field(modelGuid);
    }

    struct PolyLinePoint
    {
        glm::vec3 pos;
        f32 distanceToHere;
        glm::vec3 dir;
        f32 distance; // distance at next point
    };

    void updateMesh(Scene* scene);
    void deformMeshAlongPath(Mesh* sourceMesh, Mesh* outputMesh, f32 meshScale,
            Array<PolyLinePoint> const& polyLine, f32 pathLength);

    void onCreate(Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onBatch(class Batcher& batcher) override;
    void onRenderOutline(RenderWorld* rw, Scene* scene, f32 deltaTime);
};
