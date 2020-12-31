#pragma once

#include "renderer.h"
#include "model.h"
#include "material.h"

class Batcher
{
    struct BatchableItem
    {
        Mat4 transform;
        Mesh* mesh;
    };
    Map<Material*, Array<BatchableItem>> materialMap;

public:
    struct Batch
    {
        Material* material;
        Mesh mesh;
    };

    Array<Batch> batches;

    ~Batcher()
    {
        for (auto& batch : batches)
        {
            batch.mesh.destroy();
        }
    }

    void begin()
    {
        for (auto& batch : batches)
        {
            batch.mesh.destroy();
        }
        materialMap.clear();
    }

    void add(Material* material, Mat4 const& transform, Mesh* mesh)
    {
        materialMap[material].push({ transform, mesh });
    }

    void end(bool keepMeshData=false);

    void render(RenderWorld* rw, Mat4 const& transform=Mat4(1.f))
    {
        for (auto& batch : batches)
        {
            batch.material->draw(rw, transform, &batch.mesh);
        }
    }
};
