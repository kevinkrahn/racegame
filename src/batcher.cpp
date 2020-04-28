#include "batcher.h"

void Batcher::end()
{
    for (auto& itemsForThisMaterial : materialMap)
    {
        Mesh bigBatchedMesh;
        bigBatchedMesh.name = itemsForThisMaterial.first->name + " Batch";
        bigBatchedMesh.numVertices = 0;
        bigBatchedMesh.numIndices = 0;
        u32 vertexElementCount = 0;

        Mesh* meshAttributeSource = itemsForThisMaterial.second.front().mesh;
        bigBatchedMesh.numColors = meshAttributeSource->numColors;
        bigBatchedMesh.numTexCoords = meshAttributeSource->numTexCoords;
        bigBatchedMesh.elementSize = meshAttributeSource->elementSize;
        bigBatchedMesh.stride = meshAttributeSource->stride;

        for (auto& item : itemsForThisMaterial.second)
        {
            vertexElementCount += (u32)item.mesh->vertices.size();
            bigBatchedMesh.numVertices += item.mesh->numVertices;
            bigBatchedMesh.numIndices += item.mesh->numIndices;
        }

        bigBatchedMesh.vertices.resize(vertexElementCount);
        bigBatchedMesh.indices.resize(bigBatchedMesh.numIndices);
        u32 vertexElementIndex = 0;
        u32 indicesCopied = 0;
        u32 verticesCopied = 0;
        for (auto& item : itemsForThisMaterial.second)
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

                p = item.transform * glm::vec4(p, 1.f);
                n = glm::normalize(normalMatrix * n);

                bigBatchedMesh.vertices[vertexElementIndex + 0] = p.x;
                bigBatchedMesh.vertices[vertexElementIndex + 1] = p.y;
                bigBatchedMesh.vertices[vertexElementIndex + 2] = p.z;
                bigBatchedMesh.vertices[vertexElementIndex + 3] = n.x;
                bigBatchedMesh.vertices[vertexElementIndex + 4] = n.y;
                bigBatchedMesh.vertices[vertexElementIndex + 5] = n.z;

                for (u32 attrIndex = 6; attrIndex < item.mesh->stride / sizeof(f32); ++attrIndex)
                {
                    bigBatchedMesh.vertices[vertexElementIndex+attrIndex] = item.mesh->vertices[j+attrIndex];
                }

                vertexElementIndex += item.mesh->stride / sizeof(f32);
            }
            verticesCopied += item.mesh->numVertices;
        }

        bigBatchedMesh.createVAO();
        // TODO: the vertices and indices can be cleared because they will not be used after
        // the data is uploaded to the GPU

        batches.push_back({ itemsForThisMaterial.first, std::move(bigBatchedMesh) });
    }
    materialMap.clear();
    print("Built ", batches.size(), " batches\n");
}
