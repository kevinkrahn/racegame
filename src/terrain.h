#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"
#include "gl.h"

class Terrain : public Renderable, public Entity
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };

    f32 tileSize = 2.0f;
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::vector<f32> heightBuffer;
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    GLuint vao = 0, vbo = 0, ebo = 0;
    glm::vec3 brushSettings;
    glm::vec3 brushPosition;

    RandomSeries randomSeries;

    bool isDirty = true;
    bool isCollisionMeshDirty = true;

    PxRigidStatic* actor = nullptr;
    std::unique_ptr<struct ActorUserData> physicsUserData;
    void setDirty()
    {
        isDirty = true;
        isCollisionMeshDirty = true;
    }

public:
    Terrain()
    {
        resize(-256, -256, 256, 256);
        //generate();
        createBuffers();
    }
    Terrain(f32 x1, f32 y1, f32 x2, f32 y2)
    {
        resize(x1, y1, x2, y2);
        createBuffers();
    }
    ~Terrain()
    {
        glDeleteBuffers(0, &vbo);
        glDeleteBuffers(0, &ebo);
        glDeleteVertexArrays(0, &vao);
    }

    void raise(glm::vec2 pos, f32 radius, f32 falloff, f32 amount);
    void perturb(glm::vec2 pos, f32 radius, f32 falloff, f32 amount);
    void flatten(glm::vec2 pos, f32 radius, f32 falloff, f32 amount, f32 z);
    void smooth(glm::vec2 pos, f32 radius, f32 falloff, f32 amount);
    void erode(glm::vec2 pos, f32 radius, f32 falloff, f32 amount);
    void matchTrack(glm::vec2 pos, f32 radius, f32 falloff, f32 amount, class Scene* scene);

    void generate(f32 heightScale=4.f, f32 scale=0.05f);
    void createBuffers();
    void resize(f32 x1, f32 y1, f32 x2, f32 y2);
    f32 getZ(glm::vec2 pos) const;
    //bool containsPoint(glm::vec2 p) const { return p.x >= x1 && p.y >= y1 && p.x <= x2 && p.y <= y2; };
    i32 getCellX(f32 x) const;
    i32 getCellY(f32 y) const;
    glm::vec3 computeNormal(u32 width, u32 height, u32 x, u32 y);
    void regenerateMesh();
    void regenerateCollisionMesh(class Scene* scene);

    i32 getPriority() const override { return 100; }

    // entity
    void onCreate(class Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;

    // renderable
    //void onBeforeRender(f32 deltaTime) override {};
    void onShadowPass(class Renderer* renderer) override;
    void onDepthPrepass(class Renderer* renderer) override;
    void onLitPass(class Renderer* renderer) override;

    std::string getDebugString() const override { return "Terrain"; }
    void setBrushSettings(f32 brushRadius, f32 brushFalloff, f32 brushStrength, glm::vec3 brushPosition)
    {
        this->brushSettings = { brushRadius, brushFalloff, brushStrength };
        this->brushPosition = brushPosition;
    }
};
