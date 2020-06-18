#pragma once

#include "misc.h"
#include "math.h"
#include "entity.h"
#include "gl.h"

class Terrain : public Entity
{
    struct Vertex
    {
        Vec3 position;
        Vec3 normal;
        u32 blend;
    };

    OwnedPtr<f32[]> heightBuffer;
    OwnedPtr<Vertex[]> vertices;
    OwnedPtr<u32[]> indices;
    OwnedPtr<u32[]> blend;
	u32 heightBufferSize = 0;
	u32 indexCount = 0;

    GLuint vao = 0, vbo = 0, ebo = 0;
    Vec3 brushSettings = { 1.f, 1.f, 1.f };
    Vec3 brushPosition = { 0, 0, 1000000 };

    RandomSeries randomSeries;

    bool isDirty = true;
    bool isCollisionMeshDirty = true;

    PxMaterial* materials[2];
    OwnedPtr<PxMaterialTableIndex[]> materialIndices;
    PxRigidStatic* actor = nullptr;
    ActorUserData physicsUserData;
    void setDirty()
    {
        isDirty = true;
        isCollisionMeshDirty = true;
    }

    static constexpr u8 OFFROAD_THRESHOLD = 170;

    ShaderHandle depthShader = getShaderHandle("terrain", {{ "DEPTH_ONLY" }});
    ShaderHandle colorShaderWithBrush = getShaderHandle("terrain", { {"BRUSH_ENABLED"} });
    ShaderHandle colorShader = getShaderHandle("terrain", {});

public:
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    f32 tileSize = 2.0f;

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

    void raise(Vec2 pos, f32 radius, f32 falloff, f32 amount);
    void perturb(Vec2 pos, f32 radius, f32 falloff, f32 amount);
    void flatten(Vec2 pos, f32 radius, f32 falloff, f32 amount, f32 z);
    void smooth(Vec2 pos, f32 radius, f32 falloff, f32 amount);
    void erode(Vec2 pos, f32 radius, f32 falloff, f32 amount);
    void matchTrack(Vec2 pos, f32 radius, f32 falloff, f32 amount, class Scene* scene);
    void paint(Vec2 pos, f32 radius, f32 falloff, f32 amount, u32 materialIndex);

    void generate(f32 heightScale=4.f, f32 scale=0.05f);
    void createBuffers();
    void resize(f32 x1, f32 y1, f32 x2, f32 y2, bool preserve=false);
    f32 getZ(Vec2 pos) const;
    //bool containsPoint(Vec2 p) const { return p.x >= x1 && p.y >= y1 && p.x <= x2 && p.y <= y2; };
    i32 getCellX(f32 x) const;
    i32 getCellY(f32 y) const;
    Vec3 computeNormal(u32 width, u32 height, u32 x, u32 y);
    void regenerateMesh();
    void regenerateCollisionMesh(class Scene* scene);

    enum TerrainType
    {
        GRASS,
        SAND,
        SNOW,
        LAVA,
        MAX
    };
    i32 terrainType = TerrainType::GRASS;

    struct SurfaceMaterial
    {
        const char* name = "";
        const char* textureNames[4];
        struct Texture* textures[4];
        f32 texScale[4] = { 0.1f, 0.1f, 0.1f, 0.1f };
    } surfaceMaterials[TerrainType::MAX];

    SurfaceMaterial const& getSurfaceMaterial() const { return surfaceMaterials[terrainType]; }

    bool isOffroadAt(f32 x, f32 y) const;

    // entity
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void serializeState(Serializer& s) override;
    void applyDecal(class Decal& decal) override;

    void setBrushSettings(f32 brushRadius, f32 brushFalloff, f32 brushStrength, Vec3 brushPosition)
    {
        this->brushSettings = { brushRadius, brushFalloff, brushStrength };
        this->brushPosition = brushPosition;
    }
};
