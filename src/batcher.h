#pragma once

#include "renderer.h"
#include "model.h"
#include "material.h"

class Batcher
{
    struct BatchableItem
    {
        glm::mat4 transform;
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

    void add(Material* material, glm::mat4 const& transform, Mesh* mesh)
    {
        materialMap[material].push_back({ transform, mesh });
    }

    void end(bool keepMeshData=false);

    void render(RenderWorld* rw, glm::mat4 const& transform=glm::mat4(1.f))
    {
        for (auto& batch : batches)
        {
            batch.material->draw(rw, transform, &batch.mesh);
        }
    }
};
