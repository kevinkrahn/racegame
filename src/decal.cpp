#include "decal.h"
#include "renderer.h"

struct DecalVertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
};

void Decal::begin(Mat4 const& transform, bool worldSpace)
{
    setTransform(transform);
    this->worldSpace = worldSpace;
    mesh.destroy();
    mesh.vertices.reserve(128 * sizeof(DecalVertex) / sizeof(f32));
    mesh.indices.reserve(128);
}

void Decal::setTexture(Texture* tex, Texture* texNormal)
{
    this->tex = tex;
    this->texNormal = texNormal ? texNormal : &g_res.identityNormal;
}

void Decal::addMesh(f32* verts, u32 stride, u32* indices, u32 indexCount, Mat4 const& meshTransform)
{
    //print("Adding mesh with ", indexCount * 3, " vertices\n");

    const Vec3 planes[] = {
        {  1.f,  0.f,  0.f },
        { -1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f,  1.f },
        {  0.f,  0.f, -1.f }
    };

    Mat3 worldSpaceNormalTransform = inverseTranspose(Mat3(transform));
    auto addVertex = [&](DecalVertex vert)
    {
        vert.uv = Vec2(vert.pos.y, vert.pos.z) + 0.5f;
        if (worldSpace)
        {
            vert.pos = Vec3(transform * Vec4(vert.pos, 1.f));
            vert.normal = worldSpaceNormalTransform * vert.normal;
        }

        mesh.vertices.push_back(vert.pos.x);
        mesh.vertices.push_back(vert.pos.y);
        mesh.vertices.push_back(vert.pos.z);
        mesh.vertices.push_back(vert.normal.x);
        mesh.vertices.push_back(vert.normal.y);
        mesh.vertices.push_back(vert.normal.z);
        mesh.vertices.push_back(vert.uv.x);
        mesh.vertices.push_back(vert.uv.y);

        mesh.indices.push_back(mesh.indices.size());
    };

    Mat4 vertTransform = inverse(transform) * meshTransform;
    Mat3 normalTransform = inverseTranspose(Mat3(vertTransform));

    for (u32 i=0; i<indexCount; i+=3)
    {
        u32 j = indices[i+0] * stride / sizeof(f32);
        Vec3 v1(verts[j+0], verts[j+1], verts[j+2]);
        Vec3 n1(verts[j+3], verts[j+4], verts[j+5]);
        j = indices[i+1] * stride / sizeof(f32);
        Vec3 v2(verts[j+0], verts[j+1], verts[j+2]);
        Vec3 n2(verts[j+3], verts[j+4], verts[j+5]);
        j = indices[i+2] * stride / sizeof(f32);
        Vec3 v3(verts[j+0], verts[j+1], verts[j+2]);
        Vec3 n3(verts[j+3], verts[j+4], verts[j+5]);

        // convert vertex positions to decal local space
        v1 = Vec3(vertTransform * Vec4(v1, 1.0));
        v2 = Vec3(vertTransform * Vec4(v2, 1.0));
        v3 = Vec3(vertTransform * Vec4(v3, 1.0));

        // skip triangles that are facing away
        // TODO: fix this calculation, as it appears to remove triangles that is should not
        /*
        Vec3 normal = cross(v2 - v1, v3 - v1);
        if (normal.x > 0.f) continue;
        */

        // transform normals
        n1 = normalTransform * n1;
        n2 = normalTransform * n2;
        n3 = normalTransform * n3;

        const u32 MAX_VERTS = 9;
        SmallArray<DecalVertex, MAX_VERTS> out = { { v1, n1 }, { v2, n2 }, { v3, n3 } };

        for (u32 i=0; i<6; ++i)
        {
            if (out.empty())
            {
                break;
            }

            SmallArray<DecalVertex, MAX_VERTS> in = out;
            out.clear();

            DecalVertex lastVert = in.back();

            for (auto const& v : in)
            {
                Vec3 n = planes[i];
                Vec3 p = n * -0.5f;
                f32 d1 = dot(n, p - lastVert.pos);
                f32 d2 = dot(n, p - v.pos);
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
}

void Decal::addMesh(Mesh* mesh, Mat4 const& meshTransform)
{
    if (mesh->octree)
    {
        Array<u32> indices;
        indices.reserve(128);
        mesh->intersect(meshTransform, getBoundingBox(), indices);
        addMesh(mesh->vertices.data(), mesh->stride, indices.data(),
                (u32)indices.size(), meshTransform);
    }
    else
    {
        addMesh(mesh->vertices.data(), mesh->stride, mesh->indices.data(),
                (u32)mesh->indices.size(), meshTransform);
    }
}

void Decal::end()
{
    mesh.hasTangents = false;
    mesh.vertexFormat = {
        { 0, VertexAttributeType::FLOAT3 },
        { 1, VertexAttributeType::FLOAT3 },
        { 3, VertexAttributeType::FLOAT2 },
    };
    mesh.stride = sizeof(DecalVertex);
    mesh.numVertices = mesh.vertices.size() / (sizeof(DecalVertex) / sizeof(f32));
    mesh.numIndices = mesh.indices.size();
    mesh.createVAO();
    mesh.vertices.clear();
    mesh.vertices.shrink_to_fit();
    mesh.indices.clear();
    mesh.indices.shrink_to_fit();
}

void Decal::draw(RenderWorld* rw)
{
    static ShaderHandle shader = getShaderHandle("mesh_decal", {}, RenderFlags::DEPTH_READ, -500.f);
    auto render = [](void* renderData){
        Decal* decal = (Decal*)renderData;
        glBindTextureUnit(0, decal->tex->handle);
        glBindTextureUnit(6, decal->texNormal->handle);
        glBindVertexArray(decal->mesh.vao);
        glUniformMatrix4fv(0, 1, GL_FALSE, decal->transform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, decal->normalTransform.valuePtr());
        glUniform4fv(2, 1, (f32*)&decal->color);
        glDrawElements(GL_TRIANGLES, decal->mesh.numIndices, GL_UNSIGNED_INT, 0);
    };
    rw->transparentPass({ shader, priority, this, render });
}
