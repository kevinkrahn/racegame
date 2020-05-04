#pragma once

#include "gl.h"
#include "resources.h"
#include "math.h"
#include "smallvec.h"
#include "decal.h"
#include "renderable.h"
#include "dynamic_buffer.h"
#include "buffer.h"

#include <algorithm>
#include <vector>

enum
{
    STENCIL_ENVIRONMENT = 0,
    STENCIL_OUTLINE = 1,
    STENCIL_VEHICLE = 2,
    STENCIL_NO_OUTLINE = 4,
};

GLuint emptyVAO;

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
    //{ 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
    { 35, { 0.5f, 1.0f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } } },
};

struct Framebuffers
{
    GLuint mainFramebuffer;
    GLuint mainColorTexture;
    GLuint mainDepthTexture;

    GLuint finalColorTexture;
    GLuint finalFramebuffer;

    GLuint msaaResolveFromFramebuffer;
    GLuint msaaResolveFramebuffer;
    GLuint msaaResolveColorTexture;
    GLuint msaaResolveDepthTexture;

    GLuint stencilViewTexture;

    SmallVec<GLuint> bloomFramebuffers;
    SmallVec<GLuint> bloomColorTextures;
    SmallVec<glm::ivec2> bloomBufferSize;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;

    GLuint cszFramebuffers[5];
    GLuint cszTexture;

    GLuint saoFramebuffer;
    GLuint saoTexture;
    GLuint saoBlurTexture;

    GLuint pickFramebuffer;
    GLuint pickIDTexture;
    GLuint pickDepthTexture;

    u32 renderWidth;
    u32 renderHeight;
};

struct FullscreenFramebuffers
{
    GLuint fullscreenTexture;
    GLuint fullscreenFramebuffer;
    GLuint fullscreenBlurTextures[2];
    GLuint fullscreenBlurFramebuffer;
};

struct WorldInfo
{
    glm::mat4 orthoProjection;
    glm::vec3 sunDirection;
    f32 time;
    glm::vec3 sunColor;
    f32 pad;
    glm::mat4 cameraViewProjection;
    glm::mat4 cameraProjection;
    glm::mat4 cameraView;
    glm::vec4 cameraPosition;
    glm::mat4 shadowViewProjectionBias;
};

struct HighlightMesh
{
    Mesh* mesh;
    glm::mat4 worldTransform;
    u32 id;
};

class RenderWorld
{
    friend class Renderer;

    u32 width = 0, height = 0;
    u32 settingsVersion = 0;
    glm::vec4 clearColor = { 0.15f, 0.35f, 0.9f, 1.f };
    bool clearColorEnabled = false;

    const char* name = "";
    WorldInfo worldInfo;
    SmallVec<Framebuffers, MAX_VIEWPORTS> fbs;
    SmallVec<Camera, MAX_VIEWPORTS> cameras = { {} };
    SmallVec<DynamicBuffer, MAX_VIEWPORTS> worldInfoUBO;
    SmallVec<DynamicBuffer, MAX_VIEWPORTS> worldInfoUBOShadow;
    glm::vec4 highlightColor[MAX_VIEWPORTS] = {};

    // TODO: calculate these based on render resolution
    u32 firstBloomDivisor = 2;
    u32 lastBloomDivisor = 16;

    struct QueuedRenderable
    {
        i32 priority;
        Renderable* renderable;
    };
    std::vector<QueuedRenderable> renderables;
    std::vector<HighlightMesh> highlightMeshes;

    Texture tex[MAX_VIEWPORTS];
    u32 shadowMapResolution = 0;
    bool bloomEnabled = false;
    Buffer tempRenderBuffer = Buffer(megabytes(400), 32);
    struct PickPixelResult
    {
        glm::vec2 pickPos;
        GLuint pbo = 0;
        GLsync fence = 0;
        u32 pickID = 0;
        bool sent = false;
        bool ready = false;
    };
    SmallVec<PickPixelResult> pickPixelResults;
    bool isPickPixelPending = false;

    void setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow, u32 cameraIndex);
    void renderViewport(Renderer* renderer, u32 cameraIndex, f32 deltaTime);
    void render(Renderer* renderer, f32 deltaTime);

public:
    RenderWorld() {}
    RenderWorld(u32 width, u32 height, const char* name)
        : width(width), height(height), name(name)
    {
        for (u32 i=0; i<MAX_VIEWPORTS; ++i)
        {
            tex[i].width = width;
            tex[i].height = height;
        }
        createFramebuffers();
    }

    void add(Renderable* renderable)
    {
        renderables.push_back({ renderable->getPriority(), renderable });
    }
    template <typename T>
    T* push(T&& renderable)
    {
        u8* mem = tempRenderBuffer.bump(sizeof(T));
        new (mem) T(std::move(renderable));
        T* ptr = reinterpret_cast<T*>(mem);
        add(ptr);
        return ptr;
    }

    void addHighlightMesh(Mesh* mesh, glm::mat4 const& worldTransform, u32 id)
    {
        highlightMeshes.push_back({ mesh, worldTransform, id });
    }

    Texture* getTexture(u32 cameraIndex=0) { return &tex[cameraIndex]; }
    Texture releaseTexture(u32 cameraIndex=0);

    void setName(const char* name) { this->name = name; }
    void setViewportCount(u32 viewports);
    u32 getViewportCount() const { return cameras.size(); }
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane=0.5f, f32 farPlane=500.f, f32 fov=0.f);
    Camera& getCamera(u32 index) { return cameras[index]; }
    u32 getWidth() const { return width; }
    u32 getHeight() const { return height; }
    void setSize(u32 width, u32 height)
    {
        if (this->width != width || this->height != height)
        {
            this->width = width;
            this->height = height;
            createFramebuffers();
        }
    }
    void setClearColor(bool enabled, glm::vec4 const& color={ 0.15f, 0.35f, 0.9f, 1.f })
    {
        clearColorEnabled = enabled;
        clearColor = color;
    }

    //void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    //void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void updateWorldTime(f64 time);
    void createFramebuffers();
    void clear();
    void setHighlightColor(u32 viewportIndex, glm::vec4 const& color)
    {
        highlightColor[viewportIndex] = color;
    }

    void pickPixel(glm::vec2 pos)
    {
        PickPixelResult result;
        result.pickPos = pos;
        glCreateBuffers(1, &result.pbo);
        glNamedBufferStorage(result.pbo, sizeof(u32), nullptr, 0);
        pickPixelResults.push_back(result);
        isPickPixelPending = true;
    }
    u32* getPickPixelResult()
    {
        for (auto& result : pickPixelResults)
        {
            if (result.ready)
            {
                return &result.pickID;
            }
        }
        return nullptr;
    }
};

class Renderer
{
private:
    FullscreenFramebuffers fsfb = { 0 };
    RenderWorld renderWorld;

    u32 renderablesCount = 0;
    u32 currentRenderingCameraIndex = 0;

    u32 fullscreenBlurDivisor = 4;
    u32 settingsVersion = 0;

    std::map<std::string, GLuint> shaderPrograms;

    struct QueuedRenderable2D
    {
        i32 priority;
        Renderable2D* renderable;
    };
    std::vector<QueuedRenderable2D> renderables2D;
    std::vector<RenderWorld*> renderWorlds;

    void glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines);

    void createFullscreenFramebuffers();

public:
    void add2D(Renderable2D* renderable, i32 priority=0)
    {
        renderables2D.push_back({ priority, renderable });
    }
    template <typename T>
    T* push2D(T&& renderable, i32 priority=0)
    {
        u8* mem = renderWorld.tempRenderBuffer.bump(sizeof(T));
        new (mem) T(std::move(renderable));
        T* ptr = reinterpret_cast<T*>(mem);
        add2D(ptr, priority);
        return ptr;
    }

    u32 getCurrentRenderingCameraIndex() const { return currentRenderingCameraIndex; }
    void setCurrentRenderingCameraIndex(u32 index) { currentRenderingCameraIndex = index; }
    void init();
    void initShaders();
    void updateFramebuffers();
    void updateFullscreenFramebuffers();
    void loadShader(std::string filename, SmallVec<std::string> defines={}, std::string name="");
    u32 getShader(const char* name, i32 viewportCount=0) const;
    GLuint getShaderProgram(const char* name) const;
    void render(f32 deltaTime);
    RenderWorld* getRenderWorld() { return &renderWorld; }
    void addRenderWorld(RenderWorld* rw) { renderWorlds.push_back(rw); }
    void updateSettingsVersion()
    {
        ++settingsVersion;
        initShaders();
        updateFramebuffers();
        renderWorld.settingsVersion = settingsVersion;
    }

    size_t getTempRenderBufferSize() const { return renderWorld.tempRenderBuffer.pos; }
    u32 getRenderablesCount() { return renderablesCount; }
};

