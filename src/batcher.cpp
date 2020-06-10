#include "batcher.h"

void Batcher::end(bool keepMeshData)
{
    for (auto& itemsForThisMaterial : materialMap)
    {
        Mesh bigBatchedMesh;
        bigBatchedMesh.name = itemsForThisMaterial.key->name + " Batch";
        bigBatchedMesh.numVertices = 0;
        bigBatchedMesh.numIndices = 0;
        bigBatchedMesh.numColors = 1;
        bigBatchedMesh.numTexCoords = 1;
        bigBatchedMesh.hasTangents = true;
        bigBatchedMesh.calculateVertexFormat();

        u32 vertexElementCount = 0;
        for (auto& item : itemsForThisMaterial.value)
        {
            vertexElementCount += item.mesh->numVertices * (bigBatchedMesh.stride / sizeof(f32));
            bigBatchedMesh.numVertices += item.mesh->numVertices;
            bigBatchedMesh.numIndices += item.mesh->numIndices;
        }

        bigBatchedMesh.vertices.resize(vertexElementCount);
        bigBatchedMesh.indices.resize(bigBatchedMesh.numIndices);
        u32 vertexElementIndex = 0;
        u32 indicesCopied = 0;
        u32 verticesCopied = 0;
        for (auto& item : itemsForThisMaterial.value)
        {
            for (u32 i=0; i<item.mesh->numIndices; ++i)
            {
                bigBatchedMesh.indices[i + indicesCopied] = item.mesh->indices[i] + verticesCopied;
            }
            indicesCopied += item.mesh->numIndices;

            glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(item.transform));
            for (u32 i=0; i<item.mesh->numVertices; ++i)
            {
                u32 j = i * item.mesh->stride / sizeof(f32);
                glm::vec3 p(item.mesh->vertices[j+0], item.mesh->vertices[j+1], item.mesh->vertices[j+2]);
                glm::vec3 n(item.mesh->vertices[j+3], item.mesh->vertices[j+4], item.mesh->vertices[j+5]);
                glm::vec3 t(item.mesh->vertices[j+6], item.mesh->vertices[j+7], item.mesh->vertices[j+8]);

                p = item.transform * glm::vec4(p, 1.f);
                n = glm::normalize(normalMatrix * n);

                bigBatchedMesh.vertices[vertexElementIndex + 0] = p.x;
                bigBatchedMesh.vertices[vertexElementIndex + 1] = p.y;
                bigBatchedMesh.vertices[vertexElementIndex + 2] = p.z;
                bigBatchedMesh.vertices[vertexElementIndex + 3] = n.x;
                bigBatchedMesh.vertices[vertexElementIndex + 4] = n.y;
                bigBatchedMesh.vertices[vertexElementIndex + 5] = n.z;

                if (item.mesh->hasTangents)
                {
                    t = glm::normalize(normalMatrix * t);
                    bigBatchedMesh.vertices[vertexElementIndex + 6] = t.x;
                    bigBatchedMesh.vertices[vertexElementIndex + 7] = t.y;
                    bigBatchedMesh.vertices[vertexElementIndex + 8] = t.z;
                    for (u32 attrIndex = 9; attrIndex < item.mesh->stride / sizeof(f32); ++attrIndex)
                    {
                        bigBatchedMesh.vertices[vertexElementIndex+attrIndex] = item.mesh->vertices[j+attrIndex];
                    }
                }
                else
                {
                    bigBatchedMesh.vertices[vertexElementIndex + 6] = 0.f;
                    bigBatchedMesh.vertices[vertexElementIndex + 7] = 0.f;
                    bigBatchedMesh.vertices[vertexElementIndex + 8] = 1.f;
                    bigBatchedMesh.vertices[vertexElementIndex + 9] = 1.f;
                    bigBatchedMesh.vertices[vertexElementIndex + 10] = item.mesh->vertices[j+9];
                    bigBatchedMesh.vertices[vertexElementIndex + 11] = item.mesh->vertices[j+10];
                    bigBatchedMesh.vertices[vertexElementIndex + 12] = t.x;
                    bigBatchedMesh.vertices[vertexElementIndex + 13] = t.y;
                    bigBatchedMesh.vertices[vertexElementIndex + 14] = t.z;
                    for (u32 attrIndex = 14; attrIndex < item.mesh->stride / sizeof(f32); ++attrIndex)
                    {
                        bigBatchedMesh.vertices[vertexElementIndex+attrIndex] = item.mesh->vertices[j+attrIndex];
                    }
                }

                vertexElementIndex += bigBatchedMesh.stride / sizeof(f32);
            }
            verticesCopied += item.mesh->numVertices;
        }

        bigBatchedMesh.createVAO();

        if (!keepMeshData)
        {
            bigBatchedMesh.vertices.clear();
            bigBatchedMesh.vertices.shrink_to_fit();
            bigBatchedMesh.indices.clear();
            bigBatchedMesh.indices.shrink_to_fit();
        }

        batches.push_back({ itemsForThisMaterial.key, std::move(bigBatchedMesh) });
    }
    materialMap.clear();
}
