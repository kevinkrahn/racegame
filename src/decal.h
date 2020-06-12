#pragma once

#include "math.h"
#include "resources.h"
#include "entity.h"

class Decal
{
    glm::mat4 transform;
    glm::mat3 normalTransform;
    glm::vec4 color = { 1, 1, 1, 1 };
    Texture* tex = nullptr;
    Texture* texNormal = nullptr;
    i32 priority = TransparentDepth::TRACK_DECAL;
    Mesh mesh;
    bool worldSpace = false;

public:
    Decal() {}
    Decal(Texture* tex, glm::vec4 const& color = { 1, 1, 1, 1 }, i32 priority=TransparentDepth::TRACK_DECAL)
        : color(color), tex(tex), priority(priority) {}
    Decal(Decal const& other) {}
    Decal(Decal&& other) = default;
    ~Decal() { mesh.destroy(); }
    void setPriority(i32 priority) { this->priority = priority; }
    void setColor(glm::vec4 const& color) { this->color = color; }
    void setTexture(Texture* tex, Texture* texNormal = nullptr);
    BoundingBox getBoundingBox() const
    {
        BoundingBox bb{ glm::vec3(-0.5f), glm::vec3(0.5f) };
        return bb.transform(transform);
    }
    void begin(glm::mat4 const& transform, bool worldSpace=false);
    void addMesh(Mesh* mesh, glm::mat4 const& meshTransform);
    void addMesh(f32* verts, u32 stride, u32* indices, u32 indexCount, glm::mat4 const& meshTransform);
    void end();
    void setTransform(glm::mat4 const& transform)
    {
        this->transform = transform;
        this->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    }
    void draw(RenderWorld* rw);
};

