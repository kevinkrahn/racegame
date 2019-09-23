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
    { 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
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

    u32 msaaResolveFramebuffersCount;
    GLuint msaaResolveFromFramebuffers[MAX_VIEWPORTS];
    GLuint msaaResolveFramebuffers[MAX_VIEWPORTS];
    GLuint msaaResolveColorTexture;
    GLuint msaaResolveDepthTexture;

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

    u32 renderWidth;
    u32 renderHeight;
};

struct WorldInfo
{
    glm::mat4 orthoProjection;
    glm::vec3 sunDirection;
    f32 time;
    glm::vec3 sunColor;
    f32 pad;
    glm::mat4 cameraViewProjection[MAX_VIEWPORTS];
    glm::mat4 cameraProjection[MAX_VIEWPORTS];
    glm::mat4 cameraView[MAX_VIEWPORTS];
    glm::vec4 cameraPosition[MAX_VIEWPORTS];
    glm::mat4 shadowViewProjectionBias[MAX_VIEWPORTS];
    glm::vec4 projInfo[MAX_VIEWPORTS];
    glm::vec4 projScale;
};

class Renderer
{
private:
    Framebuffers fb = { 0 };
    WorldInfo worldInfo;
    DynamicBuffer worldInfoUBO = DynamicBuffer(sizeof(WorldInfo));
    DynamicBuffer worldInfoUBOShadow = DynamicBuffer(sizeof(WorldInfo));

    u32 width;
    u32 height;

    // TODO: calculate these based on render resolution
    u32 firstBloomDivisor = 2;
    u32 lastBloomDivisor = 16;

    std::map<std::string, u32> shaderHandleMap;
    std::vector<GLuint> loadedShaders[MAX_VIEWPORTS];

    struct QueuedRenderable
    {
        i32 priority;
        Renderable* renderable;
        bool isTemporary;
    };
    std::vector<QueuedRenderable> renderables;

    struct QueuedRenderable2D
    {
        i32 priority;
        Renderable2D* renderable;
        bool isTemporary;
    };
    std::vector<QueuedRenderable2D> renderables2D;

    SmallVec<Camera, MAX_VIEWPORTS> cameras = { {} };

    void setShadowMatrices(struct WorldInfo& worldInfo, struct WorldInfo& worldInfoShadow);
    void glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines, u32 viewportCount);
    GLuint compileShader(std::string const& filename, SmallVec<std::string> defines, u32 viewportCount);
    void createFramebuffers();

    Buffer tempRenderBuffer = Buffer(megabytes(4), 32);

public:
    void add(Renderable* renderable, bool isTemporary=false) { renderables.push_back({ renderable->getPriority(), renderable, isTemporary }); }
    void add(Renderable2D* renderable, bool isTemporary=false) { renderables2D.push_back({ renderable->getPriority(), renderable, isTemporary }); }
    template <typename T>
    T* push(T&& renderable)
    {
        u8* mem = tempRenderBuffer.bump(sizeof(T));
        new (mem) T(std::move(renderable));
        T* ptr = reinterpret_cast<T*>(mem);
        add(ptr);
        return ptr;
    }

    void init(u32 width, u32 height);
    u32 loadShader(std::string const& filename, SmallVec<std::string> defines={}, std::string name="");
    u32 getShader(const char* name, i32 viewportCount=0) const;
    GLuint getShaderProgram(const char* name, i32 viewportCount=0) const;
    void render(f32 deltaTime);
    void updateWorldTime(f64 time);

    //void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    //void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void setViewportCount(u32 viewports);
    u32 getViewportCount() const { return cameras.size(); }
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane=0.5f, f32 farPlane=500.f, f32 fov=0.f);
    Camera& getCamera(u32 index) { return cameras[index]; }

    size_t getTempRenderBufferSize() const { return tempRenderBuffer.pos; }
    std::string getDebugRenderList()
    {
        std::sort(renderables.begin(), renderables.end(), [&](auto& a, auto& b) {
            return a.priority < b.priority;
        });

        std::string result;
        std::string prev = str(renderables.front().priority, " - ", renderables.front().renderable->getDebugString());
        u32 count = 0;
        u32 items = 0;
        for (auto it = renderables.begin(); it != renderables.end(); ++it)
        {
            std::string t = str(it->priority, " - ", it->renderable->getDebugString());
            if (t != prev)
            {
                if (items != 0)
                {
                    result += '\n';
                }
                result += prev + " x " + std::to_string(count);
                prev = std::move(t);
                count = 1;
                ++items;
            }
            else
            {
                ++count;
            }

            if (it + 1 == renderables.end())
            {
                result += "\n" + prev + " x " + std::to_string(count);
            }
        }
        return result;
    }
};

