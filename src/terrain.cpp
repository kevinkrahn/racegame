#include "terrain.h"
#include "game.h"

void Terrain::resize(f32 x1, f32 y1, f32 x2, f32 y2)
{
    this->x1 = snap(x1, tileSize);
    this->y1 = snap(y1, tileSize);
    this->x2 = snap(x2, tileSize);
    this->y2 = snap(y2, tileSize);
    u32 width = (this->x2 - this->x1) / tileSize;
    u32 height = (this->y2 - this->y1) / tileSize;
    heightBuffer.reset(new f32[width * height]);
    RandomSeries s;
    for (u32 x=0; x<width; ++x)
    {
        for (u32 y=0; y<height; ++y)
        {
            //heightBuffer[i] = random(s, 0.f, 6.f);
            //heightBuffer[y * width + x] = (sinf((x + y) * 0.5f) + 1.f) * 4.f + random(s, 0.f, 1.f);
            heightBuffer[y * width + x] = 0.f;
        }
    }
}

void Terrain::render()
{
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
            game.renderer.drawLine(pos1, pos2, color, color);
            game.renderer.drawLine(pos1, pos3, color, color);
        }
    }
}

f32 Terrain::getZ(glm::vec2 pos) const
{
    u32 x = (pos.x - x1) / tileSize;
    u32 y = (pos.y - y1) / tileSize;
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    if (x < 0 || x > width) return getMinZ();
    if (y < 0 || y > height) return getMinZ();
    return heightBuffer[(y * width) + x];
}

u32 Terrain::getCellX(f32 x) const
{
    u32 width = (x2 - x1) / tileSize;
    return clamp(0u, (u32)((x - x1) / tileSize), width - 1);
}

u32 Terrain::getCellY(f32 y) const
{
    u32 height = (y2 - y1) / tileSize;
    return clamp(0u, (u32)((y - y1) / tileSize), height - 1);
}

void Terrain::raise(glm::vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    for (u32 x=minX; x<maxX; ++x)
    {
        for (u32 y=minY; y<maxY; ++y)
        {
            glm::vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 d = glm::length(pos - p);
            heightBuffer[y * width + x] += clamp((1.f - (d / radius)), 0.f, 1.f) * amount;
        }
    }
}
