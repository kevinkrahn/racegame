#pragma once

#include "renderer.h"
#include "mesh_renderables.h"
#include "model.h"
#include "material.h"
#include <vector>

class Batcher
{
    struct Batch
    {
        Material* material;
        Mesh mesh;
    };

    std::vector<Batch> batches;

    struct BatchableItem
    {
        glm::mat4 transform;
        Mesh* mesh;
    };

    std::map<Material*, std::vector<BatchableItem>> materialMap;

public:

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

    void end();

    void render(RenderWorld* rw, glm::mat4 const& transform=glm::mat4(1.f))
    {
        for (auto& batch : batches)
        {
            rw->push(LitMaterialRenderable(&batch.mesh, transform, batch.material));
        }
    }
};
