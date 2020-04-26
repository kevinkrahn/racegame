#include "spline.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "game.h"
#include "scene.h"

void Spline::onCreate(Scene* scene)
{
    updateMesh(scene);
}

void Spline::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    if (isDirty)
    {
        updateMesh(scene);
    }

#if 0
    for (size_t i=1; i<points.size(); ++i)
    {
        SplinePoint const& point = points[i];
        SplinePoint const& prevPoint = points[i-1];
        glm::vec3 prevP;
        for (f32 t=0.f; t<=1.f; t+=0.01f)
        {
            glm::vec3 p = pointOnBezierCurve(
                    prevPoint.position,
                    prevPoint.position + prevPoint.handleOffsetB,
                    point.position + point.handleOffsetA,
                    point.position, t);
            if (t > 0.f)
            {
                scene->debugDraw.line(p, prevP, blue, blue);
            }
            prevP = p;
        }
    }
#endif

    for (auto& rm : meshes)
    {
        if (rm.material)
        {
            rw->push(LitMaterialRenderable(&rm.mesh, glm::mat4(1.f), rm.material));
        }
    }
}

void Spline::updateMesh(Scene* scene)
{
    isDirty = false;

    if (actor)
    {
        for (auto& m : meshes)
        {
            if (m.collisionShape)
            {
                actor->detachShape(*m.collisionShape);
            }
        }
    }
    meshes.clear();

    if (this->points.size() < 2)
    {
        return;
    }

    Model* model = g_res.getModel(modelGuid);

    if (!actor && model->hasCollision())
    {
        actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
        physicsUserData.entityType = ActorUserData::ENTITY;
        physicsUserData.entity = nullptr;
        actor->userData = &physicsUserData;
        scene->getPhysicsScene()->addActor(*actor);
    }

    // build poly line from bezier curve
    u32 steps = 32;
    std::vector<PolyLinePoint> polyLine;
    //f32 zGroundOffset = flat ? 0.01f : 0.f;
    f32 zGroundOffset = 0.01f;
    u32 stickToGroundCollisionFlags = COLLISION_FLAG_TRACK;
    for (size_t i=0; i<points.size()-1; ++i)
    {
        SplinePoint const& point = points[i];
        SplinePoint const& nextPoint = points[i+1];
        for (u32 step=0; step<steps; ++step)
        {
            glm::vec3 pos = pointOnBezierCurve(
                    point.position,
                    point.position + point.handleOffsetB,
                    nextPoint.position + nextPoint.handleOffsetA,
                    nextPoint.position, step / (f32)steps);

            PxRaycastBuffer hit;
            if (scene->raycastStatic(pos + glm::vec3(0, 0, 3),
                        glm::vec3(0, 0, -1), 6.f, &hit, stickToGroundCollisionFlags))
            {
                pos.z = hit.block.position.z + zGroundOffset;
            }

            polyLine.push_back({ pos });
        }
    }

    glm::vec3 lastPos = points.back().position;
    PxRaycastBuffer hit;
    if (scene->raycastStatic(lastPos + glm::vec3(0, 0, 3),
                glm::vec3(0, 0, -1), 6.f, &hit, stickToGroundCollisionFlags))
    {
        lastPos.z = hit.block.position.z;
    }
    polyLine.push_back({ lastPos });

    f32 pathLength = 0;
    for (size_t i=0; i<polyLine.size()-1; ++i)
    {
        glm::vec3 diff = polyLine[i+1].pos - polyLine[i].pos;
        f32 thisPathLength = pathLength;
        pathLength += glm::length(diff);
        polyLine[i].distanceToHere = thisPathLength;
        polyLine[i].distance = pathLength;
        polyLine[i].dir = glm::normalize(diff);
    }
    polyLine.pop_back();

    for (auto& obj : model->objects)
    {
        MeshInfo meshInfo;
        Mesh* sourceMesh = &model->meshes[obj.meshIndex];
        Mesh* outputMesh = &meshInfo.mesh;
        deformMeshAlongPath(sourceMesh, outputMesh, scale, polyLine, pathLength);

        if (obj.isCollider)
        {
            meshInfo.collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(outputMesh->getCollisionMesh()),
                    *scene->railingMaterial);
            meshInfo.collisionShape->setQueryFilterData(PxFilterData(
                        COLLISION_FLAG_OBJECT, DECAL_RAILING, 0, DRIVABLE_SURFACE));
            meshInfo.collisionShape->setSimulationFilterData(PxFilterData(
                        COLLISION_FLAG_OBJECT, -1, 0, 0));
        }
        else
        {
            outputMesh->createVAO();
            meshInfo.material = g_res.getMaterial(obj.materialGuid);
        }

        meshes.emplace_back(std::move(meshInfo));
    }
}

void Spline::deformMeshAlongPath(Mesh* sourceMesh, Mesh* outputMesh, f32 meshScale,
        std::vector<PolyLinePoint> const& polyLine, f32 pathLength)
{
    f32 sourceMeshLength = (sourceMesh->aabb.max.x - sourceMesh->aabb.min.x) * meshScale;
    f32 fractionalRepeatCount = pathLength / sourceMeshLength;
    u32 totalRepeatCount = (u32)fractionalRepeatCount;
    if (fractionalRepeatCount - (u32)fractionalRepeatCount < 0.5f)
    {
        ++totalRepeatCount;
    }
    f32 lengthPerMesh = pathLength / totalRepeatCount;
    f32 sourceMeshScaleFactor = lengthPerMesh / sourceMeshLength;

    outputMesh->vertices.resize(sourceMesh->numVertices
            * sourceMesh->formatStride / sizeof(f32) * totalRepeatCount);
    outputMesh->indices.resize(sourceMesh->numIndices * totalRepeatCount);
    outputMesh->vertexFormat.clear();
    for (auto item : sourceMesh->vertexFormat)
    {
        outputMesh->vertexFormat.push_back(item);
    }
    outputMesh->stride = sourceMesh->formatStride;
    outputMesh->elementSize = sourceMesh->elementSize;
    assert(outputMesh->elementSize == 3);

    //f32 invMeshWidth = 1.f / ((sourceMesh->aabb.max.y - sourceMesh->aabb.min.y) * 8);
    f32 distanceAlongPath = 0.f;
    u32 lastPolyLinePointIndex = 0;
    for (u32 repeatCount=0; repeatCount<totalRepeatCount; ++repeatCount)
    {
        for (u32 i=0; i<sourceMesh->numVertices; ++i)
        {
            u32 j = i * sourceMesh->stride / sizeof(f32);
            glm::vec3 p(sourceMesh->vertices[j+0], sourceMesh->vertices[j+1], sourceMesh->vertices[j+2]);
            glm::vec3 n(sourceMesh->vertices[j+3], sourceMesh->vertices[j+4], sourceMesh->vertices[j+5]);
            glm::vec2 uv(sourceMesh->vertices[j+9], sourceMesh->vertices[j+10]);
            p.x -= sourceMesh->aabb.min.x;
            p *= meshScale;
            p.x *= sourceMeshScaleFactor;
            n.x *= sourceMeshScaleFactor;

            u32 pointIndex = lastPolyLinePointIndex;

            while (distanceAlongPath + p.x >= polyLine[pointIndex].distance
                    && pointIndex < polyLine.size() - 1)
            {
                ++pointIndex;
            }
            PolyLinePoint const& line = polyLine[pointIndex];

            glm::vec3 xDir = line.dir;
            glm::vec3 yDir = glm::normalize(glm::cross(glm::vec3(0, 0, 1), xDir));
            glm::vec3 zDir = glm::normalize(glm::cross(xDir, yDir));

            glm::mat3 m(1.f);
            m[0] = xDir;
            m[1] = yDir;
            m[2] = zDir;

            glm::vec3 dp = line.pos + line.dir *
                (distanceAlongPath - line.distanceToHere) + m * p;

            // bend the normal
            glm::vec3 dn = glm::normalize(m * n);

#if 0
            uv.x = (distanceAlongPath + p.x) * invMeshWidth;
#endif

            // copy over the remaining vertex attributes
            u32 nj = (repeatCount * sourceMesh->formatStride / sizeof(f32) * sourceMesh->numVertices)
                + i * sourceMesh->formatStride / sizeof(f32);
            outputMesh->vertices[nj+0] = dp.x;
            outputMesh->vertices[nj+1] = dp.y;
            outputMesh->vertices[nj+2] = dp.z;
            outputMesh->vertices[nj+3] = dn.x;
            outputMesh->vertices[nj+4] = dn.y;
            outputMesh->vertices[nj+5] = dn.z;
            for (u32 attrIndex = 6; attrIndex < sourceMesh->formatStride / sizeof(f32); ++attrIndex)
            {
                outputMesh->vertices[nj+attrIndex] = sourceMesh->vertices[j+attrIndex];
            }
#if 0
            outputMesh->vertices[nj+6] = uv.x;
            outputMesh->vertices[nj+7] = uv.y;
#endif
        }

        distanceAlongPath += lengthPerMesh;

        while (distanceAlongPath >= polyLine[lastPolyLinePointIndex].distance
                && lastPolyLinePointIndex < polyLine.size() - 1)
        {
            ++lastPolyLinePointIndex;
        }

        // copy over the indices
        for (u32 i=0; i<sourceMesh->numIndices; ++i)
        {
            outputMesh->indices[repeatCount * sourceMesh->numIndices + i] =
                sourceMesh->indices[i] + (sourceMesh->numVertices * repeatCount);
        }
    }
    outputMesh->numVertices =
        (u32)outputMesh->vertices.size() / (outputMesh->stride / sizeof(f32));
    outputMesh->numIndices =(u32)outputMesh->indices.size();
}
