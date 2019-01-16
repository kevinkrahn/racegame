#pragma once

#include "math.h"
#include "resources.h"
#include "smallvec.h"
#include <iomanip>

struct DecalVertex
{
    glm::vec3 pos;
    glm::vec2 uv;
};

std::vector<DecalVertex> createDecal(glm::mat4 const& transform, Mesh* mesh,
        glm::mat4 const& meshTransform)
{
    assert(mesh->elementSize == 3);

    std::vector<DecalVertex> outputMesh;
    outputMesh.reserve(30);

    const glm::vec3 planes[] = {
        {  1.f,  0.f,  0.f },
        { -1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f,  1.f },
        {  0.f,  0.f, -1.f }
    };

    glm::mat4 vertTransform = glm::inverse(transform) * meshTransform;
    for (u32 i=0; i<mesh->numIndices; i+=3)
    {
        u32 j = mesh->indices[i+0] * mesh->stride / sizeof(f32);
        glm::vec3 v1(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        j = mesh->indices[i+1] * mesh->stride / sizeof(f32);
        glm::vec3 v2(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);
        j = mesh->indices[i+2] * mesh->stride / sizeof(f32);
        glm::vec3 v3(mesh->vertices[j+0], mesh->vertices[j+1], mesh->vertices[j+2]);

        // convert vertex positions to decal local space
        v1 = vertTransform * glm::vec4(v1, 1.0);
        v2 = vertTransform * glm::vec4(v2, 1.0);
        v3 = vertTransform * glm::vec4(v3, 1.0);

        // skip triangles that are facing away
        /*
        glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
        if (normal.z < 0.f) continue;
        */

        SmallVec<glm::vec3, 9> out = { v1, v2, v3 };

        for (u32 i=0; i<6; ++i)
        {
            if (out.empty()) break;
            SmallVec<glm::vec3, 9> in = out;
            out.clear();

            glm::vec3 lastV = in.back();

            for (auto const& v : in)
            {
                glm::vec3 n = planes[i];
                glm::vec3 p = n * -0.5f;
                f32 d1 = glm::dot(n, p - lastV);
                f32 d2 = glm::dot(n, p - v);
                f32 d = d1 / (d1 - d2);
                if (d2 <= 0.f)
                {
                    if (d1 > 0.f)
                    {
                        out.push_back(lastV + (v - lastV) * d);
                    }
                    out.push_back(v);
                }
                else if (d1 <= 0.f)
                {
                    out.push_back(lastV + (v - lastV) * d);
                }
                lastV = v;
            }
        }

        for (u32 i=0; i<out.size(); ++i)
        {
            glm::vec3 v = out[i];
            if (i > 2)
            {
                outputMesh.push_back({ out[i-1], glm::vec2(out[i-1]) + glm::vec2(0.5) });
                outputMesh.push_back({ out.front(), glm::vec2(out.front()) + glm::vec2(0.5) });
            }
            outputMesh.push_back({ v, glm::vec2(v) + glm::vec2(0.5) });
        }
    }

    /*
    for (u32 i=0; i<outputMesh.size(); ++i)
    {
        glm::vec3 v1 = outputMesh[i+0].pos;
        glm::vec3 v2 = outputMesh[i+1].pos;
        glm::vec3 v3 = outputMesh[i+2].pos;
        glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
        print(std::fixed, std::setprecision(2));
        print(normal, '\n');
    }
    */

    return outputMesh;
}

void drawDebugDecal(glm::mat4 const& transform, Mesh* mesh, glm::mat4 const& meshTransform)
{
    auto verts = createDecal(transform, mesh, meshTransform);

    glm::vec4 offset(0, 0, 0.05f, 0);
    glm::vec4 color = { 0, 1, 0, 1 };
    for (u32 i=0; i<verts.size(); i+=3)
    {
        glm::vec3 v1 = transform * glm::vec4(verts[i+0].pos, 1.f) + offset;
        glm::vec3 v2 = transform * glm::vec4(verts[i+1].pos, 1.f) + offset;
        glm::vec3 v3 = transform * glm::vec4(verts[i+2].pos, 1.f) + offset;

        game.renderer.drawLine(v1, v2, color, color);
        game.renderer.drawLine(v2, v3, color, color);
        game.renderer.drawLine(v3, v1, color, color);
    }
}
