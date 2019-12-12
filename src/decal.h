#pragma once

#include "math.h"
#include "resources.h"
#include "smallvec.h"
#include "entity.h"

class Decal : public Renderable
{
    struct DecalVertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    glm::mat4 transform;
    glm::mat3 normalTransform;
    glm::vec3 color = { 1, 1, 1 };
    Texture* tex = nullptr;
    Texture* texNormal = nullptr;
    std::vector<DecalVertex> vertices;
    u32 vertexCount = 0;
    GLuint vao = 0, vbo = 0;
    i32 priority = 8000;

public:
    Decal() {}
    Decal(Texture* tex, glm::vec3 const& color = { 1, 1, 1 }, i32 priority=8000)
        : color(color), tex(tex), priority(priority) {}
    ~Decal()
    {
        if (vao)
        {
            glDeleteBuffers(0, &vbo);
            glDeleteVertexArrays(0, &vao);
        }
    }
    void setPriority(i32 priority) { this->priority = priority; }
    void setColor(glm::vec3 const& color) { this->color = color; }
    void setTexture(Texture* tex, Texture* texNormal = nullptr);
    BoundingBox getBoundingBox() const
    {
        BoundingBox bb{ glm::vec3(-0.5f), glm::vec3(0.5f) };
        return bb.transform(transform);
    }
    void addVertex(DecalVertex const& vert)
    {
        vertices.push_back({
            vert.pos,
            glm::normalize(vert.normal),
            glm::vec2(vert.pos.y, vert.pos.z) + glm::vec2(0.5)
        });
    }
    void begin(glm::mat4 const& transform);
    void addMesh(Mesh* mesh, glm::mat4 const& meshTransform);
    void addMesh(f32* verts, u32 stride, u32* indices, u32 indexCount, glm::mat4 const& meshTransform);
    void end();
    i32 getPriority() const override { return priority; }
    std::string getDebugString() const override { return "Decal"; }
    void onLitPassPriorityTransition(class Renderer* renderer) override;
    void onLitPass(class Renderer* renderer) override;
};

