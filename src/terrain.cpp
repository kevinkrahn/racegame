#include "terrain.h"
#include "game.h"
#include "renderer.h"
#include "debug_draw.h"
#include "scene.h"
#include "mesh_renderables.h"
#include <glm/gtc/noise.hpp>

void Terrain::createBuffers()
{
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    enum
    {
        POSITION_BIND_INDEX = 0,
        NORMAL_BIND_INDEX = 1,
        COLOR_BIND_INDEX = 2
    };

    glCreateVertexArrays(1, &vao);

    glEnableVertexArrayAttrib(vao, POSITION_BIND_INDEX);
    glVertexArrayAttribFormat(vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, POSITION_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(vao, NORMAL_BIND_INDEX);
    glVertexArrayAttribFormat(vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(vao, NORMAL_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(vao, COLOR_BIND_INDEX);
    glVertexArrayAttribFormat(vao, COLOR_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12 + 12);
    glVertexArrayAttribBinding(vao, COLOR_BIND_INDEX, 0);
}

void Terrain::generate(f32 heightScale, f32 scale)
{
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    for (u32 x = 0; x < width; ++x)
    {
        for (i32 y = 0; y < height; ++y)
        {
            heightBuffer[y * width + x] =
                glm::perlin(glm::vec2(x, y) * scale) * heightScale +
                glm::perlin(glm::vec2(x, y) * scale * 4.f) * 0.5f;
        }
    }
    setDirty();
}

void Terrain::resize(f32 x1, f32 y1, f32 x2, f32 y2)
{
    this->x1 = snap(x1, tileSize);
    this->y1 = snap(y1, tileSize);
    this->x2 = snap(x2, tileSize);
    this->y2 = snap(y2, tileSize);
    u32 width = (this->x2 - this->x1) / tileSize;
    u32 height = (this->y2 - this->y1) / tileSize;
    heightBuffer.resize(width * height);
    vertices.resize(width * height);
    for (u32 x=0; x<width; ++x)
    {
        for (u32 y=0; y<height; ++y)
        {
            heightBuffer[y * width + x] = 0.f;
        }
    }
    setDirty();
}

void Terrain::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    ActorUserData* userData = new ActorUserData;
    userData->entityType = ActorUserData::SCENERY;
    userData->entity = this;
    physicsUserData.reset(userData);
    actor->userData = userData;
    scene->getPhysicsScene()->addActor(*actor);

    regenerateMesh();
    regenerateCollisionMesh(scene);
}

void Terrain::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    /*
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    for (u32 x = 0; x < width - 1; ++x)
    {
        for (i32 y = 0; y < height - 1; ++y)
        {
            f32 z1 = heightBuffer[(y * width) + x];
            glm::vec3 pos1(x1 + x * tileSize, y1 + y * tileSize, z1);

            f32 z2 = heightBuffer[((y + 1) * width) + x];
            glm::vec3 pos2(x1 + x * tileSize, y1 + (y + 1) * tileSize, z2);

            f32 z3 = heightBuffer[(y * width) + x + 1];
            glm::vec3 pos3(x1 + (x + 1) * tileSize, y1 + y * tileSize, z3);

            const glm::vec4 color(0.f, 1.f, 0.f, 1.f);
            scene->debugDraw.line(pos1, pos2, color, color);
            scene->debugDraw.line(pos1, pos3, color, color);
        }
    }
    */

    regenerateMesh();
    renderer->add(this);
}

glm::vec3 Terrain::computeNormal(u32 width, u32 height, u32 x, u32 y)
{
    x = clamp(x, 1u, width - 2);
    y = clamp(y, 1u, height - 2);
    f32 hl = heightBuffer[y * width + x - 1];
    f32 hr = heightBuffer[y * width + x + 1];
    f32 hd = heightBuffer[(y - 1) * width + x];
    f32 hu = heightBuffer[(y + 1) * width + x];
    glm::vec3 normal(hl - hr, hd - hu, 2.f);
    return glm::normalize(normal);
}

void Terrain::regenerateMesh()
{
    if (!isDirty) { return; }
    isDirty = false;
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    indices.clear();
    for (u32 x = 0; x < width; ++x)
    {
        for (i32 y = 0; y < height; ++y)
        {
            f32 z = heightBuffer[(y * width) + x];
            glm::vec3 pos(x1 + x * tileSize, y1 + y * tileSize, z);
            glm::vec3 normal = computeNormal(width, height, x, y);
            u32 i = y * width + x;
            vertices[i] = {
                pos,
                normal,
                { 1, 1, 1 }
            };
            if (x < width - 1 && y < height - 1)
            {
                if ((x & 1) ? (y & 1) : !(y & 1))
                {
                    indices.push_back(y * width + x);
                    indices.push_back(y * width + x + 1);
                    indices.push_back((y + 1) * width + x);

                    indices.push_back((y + 1) * width + x);
                    indices.push_back(y * width + x + 1);
                    indices.push_back((y + 1) * width + x + 1);
                }
                else
                {
                    indices.push_back(y * width + x);
                    indices.push_back(y * width + x + 1);
                    indices.push_back((y + 1) * width + x + 1);

                    indices.push_back((y + 1) * width + x + 1);
                    indices.push_back((y + 1) * width + x);
                    indices.push_back(y * width + x);
                }
            }
        }
    }
    glNamedBufferData(vbo, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(ebo, indices.size() * sizeof(u32), indices.data(), GL_DYNAMIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, ebo);
}

void Terrain::regenerateCollisionMesh(Scene* scene)
{
    if (!isCollisionMeshDirty) { return; }
    isCollisionMeshDirty = false;
    PxTriangleMeshDesc desc;
    desc.points.count = vertices.size();
    desc.points.stride = sizeof(Vertex);
    desc.points.data = vertices.data();
    desc.triangles.count = indices.size() / 3;
    desc.triangles.stride = 3 * sizeof(indices[0]);
    desc.triangles.data = indices.data();

    PxDefaultMemoryOutputStream writeBuffer;
    if (!g_game.physx.cooking->cookTriangleMesh(desc, writeBuffer))
    {
        FATAL_ERROR("Failed to create collision mesh for terrain");
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* triMesh = g_game.physx.physics->createTriangleMesh(readBuffer);
    if (actor->getNbShapes() > 0)
    {
        PxShape* shape;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxTriangleMeshGeometry(triMesh));
    }
    else
    {
        PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(triMesh), *scene->offroadMaterial);
        shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_GROUND, 0, 0, DRIVABLE_SURFACE));
        shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
    }
    triMesh->release();
}

f32 Terrain::getZ(glm::vec2 pos) const
{
    u32 width = (u32)((x2 - x1) / tileSize);
    u32 height = (u32)((y2 - y1) / tileSize);
    f32 x = clamp((pos.x - x1) / tileSize - 0.5f, 0.f, width - 1.f);
    f32 y = clamp((pos.y - y1) / tileSize - 0.5f, 0.f, height - 1.f);
    u32 px = (u32)x;
    u32 py = (u32)y;

    f32 c00 = heightBuffer[(py + 0) * width + px + 0];
    f32 c10 = heightBuffer[(py + 0) * width + px + 1];
    f32 c01 = heightBuffer[(py + 1) * width + px + 0];
    f32 c11 = heightBuffer[(py + 1) * width + px + 1];

    f32 tx = x - px;
    f32 ty = y - py;

    return (1 - tx) * (1 - ty) * c00 +
        tx * (1 - ty) * c10 + (1 - tx) * ty * c01 + tx * ty * c11;
}

i32 Terrain::getCellX(f32 x) const
{
    i32 width = (x2 - x1) / tileSize;
    return clamp((i32)((x - x1) / tileSize), 0, width - 1);
}

i32 Terrain::getCellY(f32 y) const
{
    i32 height = (y2 - y1) / tileSize;
    return clamp((i32)((y - y1) / tileSize), 0, height - 1);
}

void Terrain::raise(glm::vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (x2 - x1) / tileSize;
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            glm::vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 falloff = clamp((1.f - (glm::length(pos - p) / radius)), 0.f, 1.f);
            heightBuffer[y * width + x] += falloff * amount;
        }
    }
    setDirty();
}

void Terrain::perturb(glm::vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (x2 - x1) / tileSize;
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            glm::vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 scale = 0.1f;
            f32 noise = glm::perlin(glm::vec2(x, y) * scale);
            f32 falloff = clamp((1.f - (glm::length(pos - p) / radius)), 0.f, 1.f);
            heightBuffer[y * width + x] += falloff * noise * amount;
        }
    }
    setDirty();
}

void Terrain::flatten(glm::vec2 pos, f32 radius, f32 falloff, f32 amount, f32 z)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (x2 - x1) / tileSize;
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            glm::vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 falloff = clamp((1.f - (glm::length(pos - p) / radius)), 0.f, 1.f);
            f32 currentZ = heightBuffer[y * width + x];
            heightBuffer[y * width + x] += (z - currentZ) * falloff * amount;
        }
    }
    setDirty();
}

void Terrain::smooth(glm::vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (x2 - x1) / tileSize;
    i32 height = (y2 - y1) / tileSize;
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            glm::vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 falloff = clamp((1.f - (glm::length(pos - p) / radius)), 0.f, 1.f);
            f32 hl = heightBuffer[y * width + clamp(x - 1, 0, width - 1)];
            f32 hr = heightBuffer[y * width + clamp(x + 1, 0, width - 1)];
            f32 hd = heightBuffer[clamp(y - 1, 0, height - 1) * width + x];
            f32 hu = heightBuffer[clamp(y + 1, 0, height - 1) * width + x];
            f32 currentZ = heightBuffer[y * width + x];
            f32 average = (hl + hr + hd + hu) * 0.25f;
            heightBuffer[y * width + x] += (average - currentZ) * falloff * amount;
        }
    }
    setDirty();
}

void Terrain::onShadowPass(class Renderer* renderer)
{
    glUseProgram(renderer->getShaderProgram("terrain"));
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Terrain::onDepthPrepass(class Renderer* renderer)
{
    glUseProgram(renderer->getShaderProgram("terrain"));
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Terrain::onLitPass(class Renderer* renderer)
{
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_CULL_FACE);
    glBindTextureUnit(0, g_resources.getTexture("grass")->handle);
    glBindTextureUnit(1, g_resources.getTexture("rock")->handle);
    glUseProgram(renderer->getShaderProgram("terrain"));
    glUniform3fv(3, 1, (GLfloat*)&brushSettings);
    glUniform3fv(4, 1, (GLfloat*)&brushPosition);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

