#pragma once

#include "entity.h"
#include "renderable.h"
#include "material.h"
#include "resources.h"
#include <vector>

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
    std::vector<SplinePoint> points;
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
    std::vector<MeshInfo> meshes;
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

        // TODO: remove this when tracks are updated
        if (s.deserialize && modelGuid == 0)
        {
            u32 meshTypeIndex = 0;
            s.field(meshTypeIndex);
            switch (meshTypeIndex)
            {
                case 0:
                    modelGuid = g_res.getModel("concrete_barrier")->guid;
                    break;
                case 1:
                    modelGuid = g_res.getModel("rumblestrip")->guid;
                    break;
                case 2:
                    modelGuid = g_res.getModel("railing1")->guid;
                    break;
                case 3:
                    modelGuid = g_res.getModel("railing2")->guid;
                    break;
            }
        }
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
            std::vector<PolyLinePoint> const& polyLine, f32 pathLength);

    void onCreate(Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRenderWireframe(RenderWorld* rw, Scene* scene, f32 deltaTime);
};
