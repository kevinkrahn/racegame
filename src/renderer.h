#pragma once

#include "misc.h"
#include "gl.h"
#include "resources.h"
#include "math.h"
#include "decal.h"
#include "dynamic_buffer.h"
#include "buffer.h"
#include "map.h"

struct RenderItem2D
{
    ShaderHandle shader;
    i32 priority;
    void* renderData;
    void (*render)(void*);
};

struct RenderItem
{
    void* renderData;
    void (*render)(void*);
    u8 stencil = 0;
};

struct TransparentRenderItem
{
    ShaderHandle shader;
    i32 priority;
    void* renderData;
    void (*render)(void*);
};

struct HighlightPassRenderItem
{
    void* renderData;
    void (*render)(void*);
    u8 stencil;
    u8 cameraIndex;
};

struct ShaderProgram
{
    GLuint program = 0;
    u32 renderFlags = 0;
    f32 depthOffset = 0.f;
};

GLuint emptyVAO;

struct Camera
{
    Vec3 position;
    Mat4 view;
    Mat4 projection;
    Mat4 viewProjection;
    f32 fov;
    f32 nearPlane;
    f32 farPlane;
    f32 aspectRatio;
};

const u32 MAX_VIEWPORTS = 4;
struct ViewportLayout
{
    f32 fov;
    Vec2 scale;
    Vec2 offsets[MAX_VIEWPORTS];
};

ViewportLayout viewportLayout[MAX_VIEWPORTS] = {
    { 33, { 1.0f, 1.0f }, { { 0.0f, 0.0f } } },
    //{ 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
    { 35, { 0.5f, 1.0f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f } } },
    { 22, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f } } },
    { 22, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } } },
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

    SmallArray<GLuint> bloomFramebuffers;
    SmallArray<GLuint> bloomColorTextures;
    SmallArray<Vec2i> bloomBufferSize;

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

const u32 MAX_POINT_LIGHTS = 6;
const u32 LIGHT_SPLITS = 10;

struct PointLight
{
    Vec3 position;
    f32 radius;
    Vec3 color;
    f32 falloff;
};

struct LightPartition
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    u32 pointLightCount;
    f32 pad[3];
};

struct WorldInfo
{
    Mat4 orthoProjection;
    Vec3 sunDirection;
    f32 time;
    Vec3 sunColor;
    f32 pad;
    Mat4 cameraViewProjection;
    Mat4 cameraProjection;
    Mat4 cameraView;
    Vec4 cameraPosition;
    Mat4 shadowViewProjectionBias;
    LightPartition lightPartitions[LIGHT_SPLITS][LIGHT_SPLITS];
    Vec2 invResolution;
    f32 pad2[2];
};

// TODO: profile to see if using a map improves performance vs a sorted array
struct RenderItems
{
    Map<ShaderHandle, Array<RenderItem>> depthPrepass;
    Map<ShaderHandle, Array<RenderItem>> shadowPass;
    Map<ShaderHandle, Array<RenderItem>> opaqueColorPass;
    Array<TransparentRenderItem> transparentPass;
    Map<ShaderHandle, Array<HighlightPassRenderItem>> highlightPass;
    Map<ShaderHandle, Array<RenderItem>> pickPass;
    Array<TransparentRenderItem> overlayPass;
};

class RenderWorld
{
    friend class Renderer;

    u32 width = 0, height = 0;
    u32 settingsVersion = 0;
    Vec4 clearColor = { 0.15f, 0.35f, 0.9f, 1.f };
    bool clearColorEnabled = false;
    bool pickBufferEnabled = false;

    const char* name = "";
    WorldInfo worldInfo;
    SmallArray<Framebuffers, MAX_VIEWPORTS> fbs;
    SmallArray<Camera, MAX_VIEWPORTS> cameras;
    SmallArray<DynamicBuffer, MAX_VIEWPORTS> worldInfoUBO;
    SmallArray<DynamicBuffer, MAX_VIEWPORTS> worldInfoUBOShadow;
    Vec4 highlightColor[MAX_VIEWPORTS] = {};
    Vec2 motionBlur[MAX_VIEWPORTS];
    Array<PointLight> pointLights;

    BoundingBox shadowBounds;
    bool hasCustomShadowBounds = false;

    RenderItems renderItems;

    // TODO: calculate these based on render resolution
    u32 firstBloomDivisor = 2;
    u32 lastBloomDivisor = 16;
    bool bloomEnabled = false;
    bool forceBloomOff = false;

    Texture tex[MAX_VIEWPORTS];
    u32 shadowMapResolution = 0;
    struct PickPixelResult
    {
        Vec2 pickPos;
        GLuint pbo = 0;
        GLsync fence = 0;
        u32 pickID = 0;
        bool sent = false;
        bool ready = false;
    };
    SmallArray<PickPixelResult> pickPixelResults;
    bool isPickPixelPending = false;

    void setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow, u32 cameraIndex);
    void renderViewport(class Renderer* renderer, u32 cameraIndex, f32 deltaTime);
    void render(class Renderer* renderer, f32 deltaTime);
    void partitionPointLights(u32 viewportIndex);

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

    void depthPrepass(ShaderHandle shaderHandle, RenderItem const& renderItem)
    {
        renderItems.depthPrepass[shaderHandle].push(renderItem);
    }

    void shadowPass(ShaderHandle shaderHandle, RenderItem const& renderItem)
    {
        renderItems.shadowPass[shaderHandle].push(renderItem);
    }

    void opaqueColorPass(ShaderHandle shaderHandle, RenderItem const& renderItem)
    {
        renderItems.opaqueColorPass[shaderHandle].push(renderItem);
    }

    void transparentPass(TransparentRenderItem const& renderItem)
    {
        renderItems.transparentPass.push(renderItem);
    }

    void pickPass(ShaderHandle shaderHandle, RenderItem const& renderItem)
    {
        renderItems.pickPass[shaderHandle].push(renderItem);
    }

    void highlightPass(ShaderHandle shaderHandle, HighlightPassRenderItem const& renderItem)
    {
        renderItems.highlightPass[shaderHandle].push(renderItem);
    }

    void overlayPass(TransparentRenderItem const& renderItem)
    {
        renderItems.overlayPass.push(renderItem);
    }

    Texture* getTexture(u32 cameraIndex=0) { return &tex[cameraIndex]; }
    Texture releaseTexture(u32 cameraIndex=0);

    void setBloomForceOff(bool off) { this->forceBloomOff = off; }
    void setName(const char* name) { this->name = name; }
    void setViewportCount(u32 viewports);
    u32 getViewportCount() const { return cameras.size(); }
    Camera& setViewportCamera(u32 index, Vec3 const& from, Vec3 const& to, f32 nearPlane=0.5f, f32 farPlane=500.f, f32 fov=0.f);
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
    void setClearColor(bool enabled, Vec4 const& color={ 0.15f, 0.35f, 0.9f, 1.f })
    {
        clearColorEnabled = enabled;
        clearColor = color;
    }

    void addPointLight(Vec3 const& position, Vec3 const& color, f32 radius, f32 falloff);
    //void addSpotLight(Vec3 const& position, Vec3 const& direction, Vec3 const& color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(Vec3 const& direction, Vec3 const& color);
    void setMotionBlur(u32 viewportIndex, Vec2 const& motionBlur);

    void updateWorldTime(f64 time);
    void createFramebuffers();
    void clear();
    void setHighlightColor(u32 viewportIndex, Vec4 const& color)
    {
        highlightColor[viewportIndex] = color;
    }

    void pickPixel(Vec2 pos)
    {
        PickPixelResult result;
        result.pickPos = pos;
        glCreateBuffers(1, &result.pbo);
        glNamedBufferStorage(result.pbo, sizeof(u32), nullptr, 0);
        pickPixelResults.push(result);
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
    void setShadowBounds(BoundingBox const& bb, bool enabled=true)
    {
        shadowBounds = bb;
        hasCustomShadowBounds = enabled;
    }
    void destroy();
};

class Renderer
{
private:
    FullscreenFramebuffers fsfb = { 0 };
    RenderWorld renderWorld;

    // TODO: count draw calls
    u32 currentRenderingCameraIndex = 0;

    u32 fullscreenBlurDivisor = 4;
    u32 settingsVersion = 0;

    struct ShaderProgramSource
    {
        const char* name;
        SmallArray<ShaderDefine> defines;
    };
    Array<ShaderProgram> shaderPrograms;
    Array<ShaderProgramSource> shaderProgramSources;

    Map<const char*, ShaderHandle> shaderNameMap;
    void loadShaders();
    void loadShader(const char* filename, SmallArray<const char*> defines={}, const char* name=nullptr);
    void loadShader(ShaderHandle handle);

    Array<RenderItem2D> renderItems2D;
    Array<RenderWorld*> renderWorlds;

    void createFullscreenFramebuffers();
    Buffer tempMem = Buffer(megabytes(10));

public:
    GLuint getShaderProgram(const char* name) { return shaderPrograms[shaderNameMap[name]].program; }
    GLuint getShaderProgram(ShaderHandle handle) { return shaderPrograms[handle].program; }

    ShaderProgram const& getShader(ShaderHandle handle) { return shaderPrograms[handle]; }

    void add2D(ShaderHandle shader, i32 priority, void* renderData, void(*render)(void*))
    {
        renderItems2D.push({ shader, priority, renderData, render });
    }

    template <typename T>
    void add2D(ShaderHandle shader, i32 priority, T&& render)
    {
        T* data = tempMem.bump<T>();
        new (data) T(move(render));
        auto renderFunc = [](void* renderData){ (*((T*)renderData))(); };
        renderItems2D.push({ shader, priority, (void*)data, renderFunc });
    }

    u32 getCurrentRenderingCameraIndex() const { return currentRenderingCameraIndex; }
    void setCurrentRenderingCameraIndex(u32 index) { currentRenderingCameraIndex = index; }
    void init();
    void reloadShaders();
    void updateFramebuffers();
    void updateFullscreenFramebuffers();
    ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines,
            u32 renderFlags, f32 depthOffset);
    void render(f32 deltaTime);
    RenderWorld* getRenderWorld() { return &renderWorld; }
    void addRenderWorld(RenderWorld* rw) { renderWorlds.push(rw); }
    void updateSettingsVersion()
    {
        ++settingsVersion;
        reloadShaders();
        updateFramebuffers();
        renderWorld.settingsVersion = settingsVersion;
    }
};
