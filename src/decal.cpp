#include "decal.h"
#include "renderer.h"

void Decal::begin(glm::mat4 const& transform)
{
    this->transform = transform;
    this->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    vertices.reserve(128);
}

void Decal::setTexture(Texture* tex, Texture* texNormal)
{
    this->tex = tex;
    this->texNormal = texNormal ? texNormal : &g_res.textures->identityNormal;
}

void Decal::addMesh(f32* verts, u32 stride, u32* indices, u32 indexCount, glm::mat4 const& meshTransform)
{
    //print("Adding mesh with ", indexCount * 3, " vertices\n");

    const glm::vec3 planes[] = {
        {  1.f,  0.f,  0.f },
        { -1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f,  1.f },
        {  0.f,  0.f, -1.f }
    };

    glm::mat4 vertTransform = glm::inverse(transform) * meshTransform;
    glm::mat3 normalTransform = glm::inverseTranspose(glm::mat3(vertTransform));

    for (u32 i=0; i<indexCount; i+=3)
    {
        u32 j = indices[i+0] * stride / sizeof(f32);
        glm::vec3 v1(verts[j+0], verts[j+1], verts[j+2]);
        glm::vec3 n1(verts[j+3], verts[j+4], verts[j+5]);
        j = indices[i+1] * stride / sizeof(f32);
        glm::vec3 v2(verts[j+0], verts[j+1], verts[j+2]);
        glm::vec3 n2(verts[j+3], verts[j+4], verts[j+5]);
        j = indices[i+2] * stride / sizeof(f32);
        glm::vec3 v3(verts[j+0], verts[j+1], verts[j+2]);
        glm::vec3 n3(verts[j+3], verts[j+4], verts[j+5]);

        // convert vertex positions to decal local space
        v1 = vertTransform * glm::vec4(v1, 1.0);
        v2 = vertTransform * glm::vec4(v2, 1.0);
        v3 = vertTransform * glm::vec4(v3, 1.0);

        // skip triangles that are facing away
        // TODO: fix this calculation, as it appears to remove triangles that is should not
        /*
        glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
        if (normal.x > 0.f) continue;
        */

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
}

void Decal::addMesh(Mesh* mesh, glm::mat4 const& meshTransform)
{
    assert(mesh->elementSize == 3);

    if (mesh->octree)
    {
        std::vector<u32> indices;
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
    if (vao)
    {
        glDeleteBuffers(0, &vbo);
        glDeleteVertexArrays(0, &vao);
    }

    glCreateVertexArrays(1, &vao);

    glCreateBuffers(1, &vbo);
    glNamedBufferData(vbo, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(DecalVertex));

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(vao, 1, 0);

    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, 12 + 12);
    glVertexArrayAttribBinding(vao, 2, 0);

    vertexCount = (u32)vertices.size();
    vertices.clear();
    vertices.shrink_to_fit();
}

void Decal::onLitPassPriorityTransition(Renderer* renderer)
{
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(renderer->getShaderProgram("mesh_decal"));
    glEnable(GL_CULL_FACE);
}

void Decal::onLitPass(Renderer* renderer)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.f, -500.f);
    glBindTextureUnit(0, tex->handle);
    glBindTextureUnit(6, texNormal->handle);
    glBindVertexArray(vao);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));
    glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalTransform));
    glUniform3f(2, color.x, color.y, color.z);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glDisable(GL_POLYGON_OFFSET_FILL);
}
