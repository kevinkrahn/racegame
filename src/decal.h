#pragma once

#include "math.h"
#include "resources.h"
#include "smallvec.h"

struct DecalVertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

std::vector<DecalVertex> createDecal(glm::mat4 const& transform, Mesh* mesh,
        glm::mat4 const& meshTransform)
{
    assert(mesh->elementSize == 3);

    std::vector<DecalVertex> outputMesh;
    outputMesh.reserve(30);

    auto addVertex = [&](DecalVertex const& vert) {
        outputMesh.push_back({ vert.pos, glm::normalize(vert.normal), glm::vec2(vert.pos.y, vert.pos.z) + glm::vec2(0.5) });
    };

    const glm::vec3 planes[] = {
        {  1.f,  0.f,  0.f },
        { -1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f,  1.f },
        {  0.f,  0.f, -1.f }
    };

    std::vector<u32> indices;
    BoundingBox decalBoundingBox{ glm::vec3(-0.5f), glm::vec3(0.5f) };
    mesh->intersect(meshTransform, decalBoundingBox.transform(transform), indices);

    glm::mat4 vertTransform = glm::inverse(transform) * meshTransform;
    glm::mat3 normalTransform = glm::inverseTranspose(glm::mat3(vertTransform));

#if 1
    for (u32 i=0; i<(u32)indices.size(); i+=3)
    {
        u32 j = mesh->indices[i+0] * mesh->stride / sizeof(f32);
        glm::vec3 v1(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n1(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);
        j = mesh->indices[i+1] * mesh->stride / sizeof(f32);
        glm::vec3 v2(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n2(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);
        j = mesh->indices[i+2] * mesh->stride / sizeof(f32);
        glm::vec3 v3(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n3(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);

        // convert vertex positions to decal local space
        v1 = vertTransform * glm::vec4(v1, 1.0);
        v2 = vertTransform * glm::vec4(v2, 1.0);
        v3 = vertTransform * glm::vec4(v3, 1.0);

        // transform normals
        n1 = normalTransform * n1;
        n2 = normalTransform * n2;
        n3 = normalTransform * n3;

        addVertex(DecalVertex{ v1, n1 });
        addVertex(DecalVertex{ v2, n2 });
        addVertex(DecalVertex{ v3, n3 });
    }

#else
    for (u32 i=0; i<(u32)indices.size(); i+=3)
    {
        u32 j = mesh->indices[i+0] * mesh->stride / sizeof(f32);
        glm::vec3 v1(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n1(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);
        j = mesh->indices[i+1] * mesh->stride / sizeof(f32);
        glm::vec3 v2(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n2(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);
        j = mesh->indices[i+2] * mesh->stride / sizeof(f32);
        glm::vec3 v3(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        glm::vec3 n3(mesh->vertices[j+3], mesh->vertices[j+4], mesh->vertices[j+5]);

        // convert vertex positions to decal local space
        v1 = vertTransform * glm::vec4(v1, 1.0);
        v2 = vertTransform * glm::vec4(v2, 1.0);
        v3 = vertTransform * glm::vec4(v3, 1.0);

        // skip triangles that are facing away
        glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
        if (normal.x > 0.f) continue;

        // transform normals
        n1 = normalTransform * n1;
        n2 = normalTransform * n2;
        n3 = normalTransform * n3;

        const u32 MAX_VERTS = 9;
        SmallVec<DecalVertex, MAX_VERTS> out = { { v1, n1 }, { v2, n2 }, { v3, n3 } };

        for (u32 i=0; i<6; ++i)
        {
            if (out.empty())
            {
                break;
            }

            SmallVec<DecalVertex, MAX_VERTS> in = out;
            out.clear();

            DecalVertex lastVert = in.back();

            for (auto const& v : in)
            {
                glm::vec3 n = planes[i];
                glm::vec3 p = n * -0.5f;
                f32 d1 = glm::dot(n, p - lastVert.pos);
                f32 d2 = glm::dot(n, p - v.pos);
                f32 d = d1 / (d1 - d2);
                if (d2 <= 0.f)
                {
                    if (d1 > 0.f)
                    {
                        out.push_back({
                            lastVert.pos + (v.pos - lastVert.pos) * d,
                            lastVert.normal + (v.normal - lastVert.normal) * d,
                        });
                    }
                    out.push_back(v);
                }
                else if (d1 <= 0.f)
                {
                    out.push_back({
                        lastVert.pos + (v.pos - lastVert.pos) * d,
                        lastVert.normal + (v.normal - lastVert.normal) * d,
                    });
                }
                lastVert = v;
            }
        }

        for (u32 i=0; i<out.size(); ++i)
        {
            addVertex(out[i]);
            if (i > 2)
            {
                addVertex(out.front());
                addVertex(out[i-1]);
            }
        }
    }
#endif

    return outputMesh;
}

