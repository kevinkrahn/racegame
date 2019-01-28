#pragma once

#include "resources.h"
#include "math.h"
#include "smallvec.h"
#include "decal.h"
#include "material.h"

#include <SDL2/SDL.h>
#include <vector>

const u32 MAX_BUFFERED_FRAMES = 3;

struct Camera
{
    glm::vec3 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    f32 fov;
    f32 nearPlane;
    f32 farPlane;
    f32 aspectRatio;
};

const u32 MAX_VIEWPORTS = 4;
struct ViewportLayout
{
    f32 fov;
    glm::vec2 scale;
    glm::vec2 offsets[MAX_VIEWPORTS];
};

ViewportLayout viewportLayout[MAX_VIEWPORTS] = {
    { 33, { 1.0f, 1.0f }, { { 0.0f, 0.0f } } },
    { 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } } },
};

struct RenderTextureItem
{
    u32 renderHandle;
    glm::mat4 transform;
    glm::vec3 color = glm::vec3(1.0);
    bool overwriteColor = false;
};

class Renderer
{
public:
    SDL_Window* initWindow(const char* name, u32 width, u32 height);
    u32 loadMesh(Mesh const& mesh);
    u32 loadTexture(Texture const& texture, u8* data, size_t size);
    u32 loadShader(std::string const& filename, SmallVec<std::string> defines={}, std::string name="");
    u32 getShader(const char* name) const;
    void render(f32 deltaTime);

    void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform, Material* material);
    void drawMesh(u32 renderHandle, glm::mat4 const& worldTransform, Material* material);
    void drawMeshOverlay(u32 renderHandle, u32 viewportIndex, glm::mat4 const& worldTransform, glm::vec3 const& color);

    void setViewportCount(u32 viewports);
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane=0.5f, f32 farPlane=500.f);
    Camera& getCamera(u32 index);

    void drawLine(glm::vec3 const& p1, glm::vec3 const& p2,
            glm::vec4 const& c1 = glm::vec4(1), glm::vec4 const& c2 = glm::vec4(1));
    void drawQuad2D(u32 texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1, glm::vec2 t2,
            glm::vec3 color, f32 alpha=1.f, bool colorShader=true);
    //void drawQuad2D(u32 texture, glm::vec2 p, f32 angle=0.f, glm::vec3 color=glm::vec3(1.f), f32 alpha=1.f);
    void drawBillboard(u32 texture, glm::vec3 const& position, glm::vec3 const& scale, glm::vec4 const& color, f32 angle);

    void drawTrack2D(std::vector<RenderTextureItem> const& staticItems,
                     SmallVec<RenderTextureItem, 16> const& dynamicItems, u32 width, u32 height, glm::vec2 pos);

    void drawRibbon(class Ribbon const& ribbon, u32 texture);

    void drawDecal(std::vector<DecalVertex> const& verts, glm::mat4 const& transform,
            u32 texture, glm::vec3 const& color = glm::vec3(1.f));
};
