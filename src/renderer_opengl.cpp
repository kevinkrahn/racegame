#include "renderer.h"
#include "ribbon.h"
#include "game.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

class DynamicBuffer
{
private:
    size_t size = 0;
    GLuint buffers[MAX_BUFFERED_FRAMES];
    bool created = false;
    size_t offset = 0;
    u32 bufferIndex = 0;

public:
    DynamicBuffer(size_t size) : size(size) {}

    void checkBuffers()
    {
        if (!created)
        {
            glCreateBuffers(3, buffers);
            for (u32 i=0; i<MAX_BUFFERED_FRAMES; ++i)
            {
                glNamedBufferData(buffers[i], size, nullptr, GL_DYNAMIC_DRAW);
            }
            created = true;
        }
    }

    GLuint getBuffer()
    {
        checkBuffers();
        return buffers[game.frameIndex];
    }

    void updateData(void* data, size_t range = 0)
    {
        checkBuffers();
        glNamedBufferSubData(getBuffer(), 0, range == 0 ? size : range, data);
    }

    void* map(size_t dataSize)
    {
        if (bufferIndex != game.frameIndex)
        {
            offset = 0;
            bufferIndex = game.frameIndex;
        }

        assert(offset + dataSize < size);

        void* ptr = glMapNamedBufferRange(getBuffer(), offset, dataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        offset += dataSize;
        return ptr;
    }

    void unmap()
    {
        glUnmapNamedBuffer(getBuffer());
    }
};

#if 0
class NiceMesh
{
private:
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    u32 stride = 0;

public:
    NiceMesh(std::initializer_list<u32> vertexAttributes)
    {
        glCreateVertexArrays(1, &vao);

        for (u32 i=0; i<vertexAttributes.size(); ++i)
        {
            glEnableVertexArrayAttrib(vao, i);
            glVertexArrayAttribFormat(vao, i, vertexAttributes[i], GL_FLOAT, GL_FALSE, stride);
            glVertexArrayAttribBinding(vao, i, 0);
            stride += sizeof(f32) * vertexAttributes[i];
        }
    }

    void setBuffers()
    {
        glVertexArrayVertexBuffer(vao, 0, vbo.getBuffer(), 0, stride);
        glVertexArrayElementBuffer(vao, ebo.getBuffer());
    }
};
#endif

struct GLMesh
{
    GLuint vao, vbo, ebo;
    u32 numIndices;
};

struct GLShader
{
    std::string filename;
    GLuint program;
    bool hasGeometryShader;
    SmallVec<std::string> defines;
};

struct GLTexture
{
    GLuint tex;
};

struct RenderItem
{
    u32 meshHandle;
    glm::mat4 worldTransform;
    Material* material;
};

struct RenderMeshOverlay
{
    u32 meshHandle;
    u32 viewportIndex;
    glm::mat4 worldTransform;
    glm::vec3 color;
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
} worldInfo;

struct DebugVertex
{
    glm::vec3 position;
    glm::vec4 color;
};

struct QuadPoint
{
    glm::vec2 xy;
    glm::vec2 uv;
};

struct Quad2D
{
    GLuint tex;
    QuadPoint points[4];
    glm::vec4 color;
};

struct Billboard
{
    glm::vec3 position;
    glm::vec3 scale;
    u32 texture;
    glm::vec4 color;
    f32 angle;
};

struct TrackTexture
{
    GLuint multisampleFramebuffer, multisampleTex, multisampleDepthBuffer;
    GLuint destFramebuffer, destTex;
    u32 width = 0;
    u32 height = 0;
    u32 texRenderHandle;
} track;

struct RenderInfo
{
    u32 count;
    u32 texture;
};

struct Decal
{
    glm::mat4 worldTransform;
    u32 count;
    u32 texture;
    glm::vec3 color;
};

std::map<std::string, u32> shaderHandleMap;
std::vector<GLShader> loadedShaders;
std::vector<GLMesh> loadedMeshes;
std::vector<GLTexture> loadedTextures;
std::vector<RenderItem> renderList;
std::vector<RenderMeshOverlay> renderListOverlay;
std::vector<DebugVertex> debugVertices;
std::vector<Quad2D> renderListText2D;
std::vector<Quad2D> renderListTex2D;
std::vector<Billboard> renderListBillboard;
std::vector<RenderInfo> renderListRibbon;
std::vector<Decal> renderListDecal;
SmallVec<Camera, MAX_VIEWPORTS> cameras = { {} };
Material defaultMaterial;

u32 viewportGapPixels = 1;

DynamicBuffer worldInfoUBO(sizeof(WorldInfo));
DynamicBuffer worldInfoUBOShadow(sizeof(WorldInfo));
DynamicBuffer debugVertexBuffer(sizeof(DebugVertex) * 300000);
DynamicBuffer ribbonVertexBuffer(sizeof(DebugVertex) * 50000);
DynamicBuffer decalVertexBuffer(sizeof(DecalVertex) * 10000);
GLMesh debugMesh;
GLMesh ribbonMesh;
GLMesh decalMesh;
GLuint emptyVAO;

struct Framebuffers
{
    GLuint mainFramebuffer;
    GLuint mainColorTexture;
    GLuint mainDepthTexture;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;

    GLuint cszFramebuffers[5];
    GLuint cszTexture;

    GLuint saoFramebuffer;
    GLuint saoTexture;
    GLuint saoBlurTexture;

    u32 renderWidth;
    u32 renderHeight;
} fb;

void glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines)
{
    std::ostringstream str;
    str << "#version 450\n";
    str << "#define MAX_VIEWPORTS " << MAX_VIEWPORTS << '\n';
    str << "#define VIEWPORT_COUNT " << cameras.size() << '\n';
    for (auto const& d : defines)
    {
        str << "#define " << d << '\n';
    }
    std::string tmp = str.str();
    const char* sources[] = { tmp.c_str(), src.c_str() };
    glShaderSource(shader, 2, sources, 0);
}

void compileShader(std::string const& filename, SmallVec<std::string> defines, GLShader& shader)
{
    std::ifstream file(filename);
    if (!file)
    {
        FATAL_ERROR("Cannot load shader file: ", filename);
    }

    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    std::string shaderStr = stream.str();

    // add support for #include to glsl files
    while (true)
    {
        std::string identifier = "#include";
        auto pos = shaderStr.find(identifier);
        if (pos == std::string::npos)
        {
            break;
        }
        auto offset = pos + identifier.size();
        auto newLinePos = shaderStr.find('\n', offset);
        newLinePos = (newLinePos == std::string::npos) ? shaderStr.size() - 1 : newLinePos;

        auto trim = [](std::string const& s) -> std::string {
            auto front = std::find_if_not(s.begin(),s.end(), [](int c) { return isspace(c); });
            return std::string(front, std::find_if_not(s.rbegin(), std::string::const_reverse_iterator(front), [](int c) { return isspace(c); }).base());
        };
        std::string includeStr = trim(shaderStr.substr(offset, newLinePos - offset));
        includeStr.erase(std::remove(includeStr.begin(), includeStr.end(), '"'), includeStr.end());

        fs::path includePath = fs::path(filename).parent_path() / fs::path(includeStr);

        std::ifstream file(includePath.string());
        if (!file)
        {
            FATAL_ERROR("Cannot load shader include file: ", includePath.string(), " (Included from ", filename, ")");
        }

        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        std::string includeContent = stream.str();

        shaderStr.replace(pos, newLinePos - pos, includeContent);
    }

    GLint success, errorMessageLength;
    GLuint program = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSources(vertexShader, shaderStr, defines.concat({ "VERT" }));
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetShaderInfoLog(vertexShader, errorMessageLength, 0, (GLchar*)errorMessage.data());
        error("Vertex Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
    }
    glAttachShader(program, vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSources(fragmentShader, shaderStr, defines.concat({ "FRAG" }));
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetShaderInfoLog(fragmentShader, errorMessageLength, 0, (GLchar*)errorMessage.data());
        error("Fragment Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
    }
    glAttachShader(program, fragmentShader);

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    bool hasGeometryShader = shaderStr.find("GEOM") != std::string::npos;
    if (hasGeometryShader)
    {
        glShaderSources(geometryShader, shaderStr, defines.concat({ "GEOM" }));
        glCompileShader(geometryShader);
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
            std::string errorMessage(errorMessageLength, ' ');
            glGetShaderInfoLog(geometryShader, errorMessageLength, 0, (GLchar*)errorMessage.data());
            error("Geometry Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
        }
        glAttachShader(program, geometryShader);
    }

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetProgramInfoLog(program, errorMessageLength, 0, (GLchar*)errorMessage.data());
        error("Shader Link Error: (", filename, ")\n", errorMessage, '\n');
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    shader.filename = filename;
    shader.program = program;
    shader.defines = std::move(defines);
    shader.hasGeometryShader = hasGeometryShader;
}

u32 Renderer::loadShader(std::string const& filename, SmallVec<std::string> defines, std::string name)
{
    if (name.empty())
    {
        name = fs::path(filename).stem().string();
    }

    if (shaderHandleMap.count(name) != 0)
    {
        return shaderHandleMap[name];
    }

    GLShader shader = {};
    compileShader(filename, defines, shader);
    loadedShaders.push_back(shader);
    u32 handle = loadedShaders.size() - 1;
    shaderHandleMap[name] = handle;
    return handle;
}

u32 Renderer::getShader(const char* name) const
{
    auto it = shaderHandleMap.find(name);
    assert(it != shaderHandleMap.end());
    return it->second;
}

GLuint getShaderProgram(const char* name)
{
    auto it = shaderHandleMap.find(name);
    assert(it != shaderHandleMap.end());
    return loadedShaders[it->second].program;
}

#ifndef NDEBUG
static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam)
{
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }
    print("OpenGL Debug (", id, "): ", message, '\n');
    assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
}
#endif

SDL_Window* Renderer::initWindow(const char* name, u32 width, u32 height)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR);
#endif

    SDL_Window* window = SDL_CreateWindow(name,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!window)
    {
        FATAL_ERROR("Failed to create SDL window: ", SDL_GetError())
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if(!context)
    {
        FATAL_ERROR("Failed to create OpenGL context: ", SDL_GetError())
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugOutput, nullptr);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    SDL_GL_SetSwapInterval(game.config.vsync ? 1 : 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loadShader("shaders/lit.glsl");
    loadShader("shaders/debug.glsl");
    loadShader("shaders/quad2D.glsl", { "COLOR" }, "tex2D");
    loadShader("shaders/quad2D.glsl", {}, "text2D");
    loadShader("shaders/post.glsl");
    loadShader("shaders/mesh2D.glsl");
    loadShader("shaders/billboard.glsl");
    loadShader("shaders/ribbon.glsl");
    loadShader("shaders/csz.glsl");
    loadShader("shaders/csz_minify.glsl");
    loadShader("shaders/sao.glsl");
    loadShader("shaders/sao_blur.glsl");
    loadShader("shaders/overlay.glsl");
    loadShader("shaders/mesh_decal.glsl");

    glCreateVertexArrays(1, &emptyVAO);

    GLMesh emptyMesh = {};
    emptyMesh.vao = emptyVAO;
    loadedMeshes.push_back(emptyMesh);

    // create debug vertex buffer
    glCreateVertexArrays(1, &debugMesh.vao);

    glEnableVertexArrayAttrib(debugMesh.vao, 0);
    glVertexArrayAttribFormat(debugMesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(debugMesh.vao, 0, 0);

    glEnableVertexArrayAttrib(debugMesh.vao, 1);
    glVertexArrayAttribFormat(debugMesh.vao, 1, 4, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(debugMesh.vao, 1, 0);

    // create ribbon vertex buffer
    glCreateVertexArrays(1, &ribbonMesh.vao);

    glEnableVertexArrayAttrib(ribbonMesh.vao, 0);
    glVertexArrayAttribFormat(ribbonMesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(ribbonMesh.vao, 0, 0);

    glEnableVertexArrayAttrib(ribbonMesh.vao, 1);
    glVertexArrayAttribFormat(ribbonMesh.vao, 1, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(ribbonMesh.vao, 1, 0);

    glEnableVertexArrayAttrib(ribbonMesh.vao, 2);
    glVertexArrayAttribFormat(ribbonMesh.vao, 2, 4, GL_FLOAT, GL_FALSE, 12 + 12);
    glVertexArrayAttribBinding(ribbonMesh.vao, 2, 0);

    glEnableVertexArrayAttrib(ribbonMesh.vao, 3);
    glVertexArrayAttribFormat(ribbonMesh.vao, 3, 2, GL_FLOAT, GL_FALSE, 12 + 12 + 16);
    glVertexArrayAttribBinding(ribbonMesh.vao, 3, 0);

    // create decal vertex buffer
    glCreateVertexArrays(1, &decalMesh.vao);

    glEnableVertexArrayAttrib(decalMesh.vao, 0);
    glVertexArrayAttribFormat(decalMesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(decalMesh.vao, 0, 0);

    glEnableVertexArrayAttrib(decalMesh.vao, 1);
    glVertexArrayAttribFormat(decalMesh.vao, 1, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(decalMesh.vao, 1, 0);

    glEnableVertexArrayAttrib(decalMesh.vao, 2);
    glVertexArrayAttribFormat(decalMesh.vao, 2, 2, GL_FLOAT, GL_FALSE, 12 + 12);
    glVertexArrayAttribBinding(decalMesh.vao, 2, 0);

    // main framebuffer
    const u32 layers = cameras.size();
    ViewportLayout& layout = viewportLayout[cameras.size() - 1];
    fb.renderWidth = (u32)(game.config.resolutionX * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0));
    fb.renderHeight = (u32)(game.config.resolutionY * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0));

    glGenTextures(1, &fb.mainColorTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainColorTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &fb.mainDepthTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainDepthTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &fb.mainFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.mainDepthTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.mainColorTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // csv framebuffers
    glGenTextures(1, &fb.cszTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.cszTexture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, ARRAY_SIZE(fb.cszFramebuffers) - 1);
    for (u32 i=1; i<=ARRAY_SIZE(fb.cszFramebuffers); ++i)
    {
        i32 w = i32(fb.renderWidth >> (i - 1));
        i32 h = i32(fb.renderHeight >> (i - 1));
        glTexImage3D(GL_TEXTURE_2D_ARRAY, i - 1, GL_R32F, w, h, layers, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glm::vec4 borderColor(1.f);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

    glGenFramebuffers(ARRAY_SIZE(fb.cszFramebuffers), fb.cszFramebuffers);
    for (u32 i=0; i<ARRAY_SIZE(fb.cszFramebuffers); ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.cszTexture, i);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    // sao framebuffers
    glGenTextures(1, &fb.saoTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.saoTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

    glGenTextures(1, &fb.saoBlurTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.saoBlurTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

    glGenFramebuffers(1, &fb.saoFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.saoFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.saoTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fb.saoBlurTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // shadow framebuffer
    glGenTextures(1, &fb.shadowDepthTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.shadowDepthTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
            game.config.shadowMapResolution, game.config.shadowMapResolution, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

    glGenFramebuffers(1, &fb.shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.shadowDepthTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // create a white texture
    Texture tex;
    tex.format = Texture::Format::RGBA8;
    tex.width = 1;
    tex.height = 1;
    u8 white[] = { 255, 255, 255, 255 };
    u32 index = loadTexture(tex, white, sizeof(white));

    return window;
}

static void setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow)
{
    glm::vec3 inverseLightDir = worldInfo.sunDirection;
    glm::mat4 depthView = glm::lookAt(inverseLightDir, glm::vec3(0), glm::vec3(0, 0, 1));
    for (u32 i=0; i<cameras.size(); ++i)
    {
        Camera const& cam = cameras[i];
        glm::mat4 inverseViewProj = depthView * glm::inverse(cam.viewProjection);

        glm::vec3 ndc[] = {
            { -1,  1, 0 },
            {  1,  1, 0 },
            {  1, -1, 0 },
            { -1, -1, 0 },

            { -1,  1, 1 },
            {  1,  1, 1 },
            {  1, -1, 1 },
            { -1, -1, 1 },
        };

        f32 minx =  FLT_MAX;
        f32 maxx = -FLT_MAX;
        f32 miny =  FLT_MAX;
        f32 maxy = -FLT_MAX;
        f32 minz =  FLT_MAX;
        f32 maxz = -FLT_MAX;
        for (auto& v : ndc)
        {
            glm::vec4 b = inverseViewProj * glm::vec4(v, 1.f);
            v = glm::vec3(b) / b.w;

            if (v.x < minx) minx = v.x;
            if (v.x > maxx) maxx = v.x;
            if (v.y < miny) miny = v.y;
            if (v.y > maxy) maxy = v.y;
            if (v.z < minz) minz = v.z;
            if (v.z > maxz) maxz = v.z;
        }

        glm::vec3 center = (glm::vec3(minx, miny, minz) + glm::vec3(maxx, maxy, maxz)) * 0.5f;
        f32 extent = glm::max(maxx-minx, maxy-miny) * 0.5f;
        f32 snapMultiple = 2.f * extent / game.config.shadowMapResolution;
        center.x = snap(center.x, snapMultiple);
        center.y = snap(center.y, snapMultiple);
        center.z = snap(center.z, snapMultiple);

        glm::mat4 depthProjection = glm::ortho(center.x-extent, center.x+extent,
                                            center.y+extent, center.y-extent, -maxz, -minz);
        glm::mat4 viewProj = depthProjection * depthView;

        worldInfoShadow.cameraViewProjection[i] = viewProj;
        worldInfo.shadowViewProjectionBias[i] = glm::mat4(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f
        ) * viewProj;
    }
}

void Renderer::render(f32 deltaTime)
{
    // update worldinfo uniform buffer
    worldInfo.time = (f32)game.currentTime;
    worldInfo.orthoProjection = glm::ortho(0.f, (f32)game.windowWidth, (f32)game.windowHeight, 0.f);
    u32 viewportCount = cameras.size();
    for (u32 i=0; i<viewportCount; ++i)
    {
        worldInfo.cameraViewProjection[i] = cameras[i].viewProjection;
        worldInfo.cameraProjection[i] = cameras[i].projection;
        worldInfo.cameraView[i] = cameras[i].view;
        worldInfo.cameraPosition[i] = glm::vec4(cameras[i].position, 1.0);
    }
    for (u32 i=0; i<cameras.size(); ++i)
    {
        Camera const& cam = cameras[i];
        worldInfo.projInfo[i] = {
            -2.f / (fb.renderWidth * cam.projection[0][0]),
            -2.f / (fb.renderHeight * cam.projection[1][1]),
            (1.f - cam.projection[0][2]) / cam.projection[0][0],
            (1.f + cam.projection[1][2]) / cam.projection[1][1]
        };
        const float scale = glm::abs(2.f * glm::tan(cam.fov * 0.5f));
        worldInfo.projScale[i] = fb.renderHeight / scale;
    }

    WorldInfo worldInfoShadow = worldInfo;
    setShadowMatrices(worldInfo, worldInfoShadow);

    worldInfoUBO.updateData(&worldInfo);
    worldInfoUBOShadow.updateData(&worldInfoShadow);

    // bind worldinfo with shadow matrices
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBOShadow.getBuffer());

    // shadow map
    glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);

    glViewport(0, 0, game.config.shadowMapResolution, game.config.shadowMapResolution);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBindTextureUnit(2, fb.shadowDepthTexture);
    glUseProgram(getShaderProgram("lit"));
    glEnable(GL_DEPTH_CLAMP);
    glDisable(GL_BLEND);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.f, 4096.f);
    glCullFace(GL_FRONT);
    for (auto const& r : renderList)
    {
        if (!r.material->castShadow)
        {
            continue;
        }
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }
    glCullFace(GL_BACK);
    glDisable(GL_DEPTH_CLAMP);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // bind real worldinfo
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBO.getBuffer());

    // depth prepass
    glViewport(0, 0, fb.renderWidth, fb.renderHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);

    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glUseProgram(getShaderProgram("lit"));
    for (auto const& r : renderList)
    {
        if (!r.material->depthWrite)
        {
            continue;
        }
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }
    glBindTextureUnit(1, fb.mainDepthTexture);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // generate csz texture
    glBindVertexArray(emptyVAO);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[0]);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glUseProgram(getShaderProgram("csz"));
    glm::vec4 clipInfo[MAX_VIEWPORTS];
    for (u32 i=0; i<cameras.size(); ++i)
    {
        Camera const& cam = cameras[i];
        clipInfo[i] = { cam.nearPlane * cam.farPlane, cam.nearPlane - cam.farPlane, cam.farPlane, 0.f };
    }
    glUniform4fv(0, cameras.size(), (GLfloat*)clipInfo);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTextureUnit(3, fb.cszTexture);

    // minify csz texture
    glUseProgram(getShaderProgram("csz_minify"));
    for (u32 i=1; i<ARRAY_SIZE(fb.cszFramebuffers); ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[i]);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, i32(fb.renderWidth >> i), i32(fb.renderHeight >> i));
        glUniform1i(0, i-1);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // scaleable ambient obscurance
    glViewport(0, 0, fb.renderWidth, fb.renderHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.saoFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glUseProgram(getShaderProgram("sao"));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTextureUnit(4, fb.saoTexture);

#if 1
    glUseProgram(getShaderProgram("sao_blur"));

    // sao hblur
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glUniform2i(0, 1, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTextureUnit(4, fb.saoBlurTexture);

    // sao vblur
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glUniform2i(0, 0, 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
#endif

    // color pass
    glBindTextureUnit(4, fb.saoTexture);
    glUseProgram(getShaderProgram("lit"));
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDepthFunc(GL_EQUAL);
    for (auto const& r : renderList)
    {
        if (r.material->textures.size() == 0)
        {
            glBindTextureUnit(0, loadedTextures[0].tex);
        }
        else
        {
            // TODO: support more than one texture
            glBindTextureUnit(0, loadedTextures[r.material->textures[0]].tex);
        }

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(r.worldTransform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform3f(2, r.material->lighting.color.x, r.material->lighting.color.y, r.material->lighting.color.z);

        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    // overlays
    glUseProgram(getShaderProgram("overlay"));
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    for (auto const& r : renderListOverlay)
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        glUniform3f(1, r.color.x, r.color.y, r.color.z);
        glUniform1i(2, (i32)r.viewportIndex);
        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
        renderListOverlay.clear();
    }

    // debug lines
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    if (debugVertices.size() > 0)
    {
        Camera const& camera = cameras[0];
        debugVertexBuffer.updateData(debugVertices.data(), debugVertices.size() * sizeof(DebugVertex));
        glVertexArrayVertexBuffer(debugMesh.vao, 0, debugVertexBuffer.getBuffer(), 0, sizeof(DebugVertex));

        glUseProgram(getShaderProgram("debug"));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection));

        glBindVertexArray(debugMesh.vao);
        glDrawArrays(GL_LINES, 0, debugVertices.size());
        debugVertices.clear();
    }

    // decals
    if (renderListDecal.size() > 0)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, -1000.f);
        glDepthMask(GL_FALSE);
        glUseProgram(getShaderProgram("mesh_decal"));

        glVertexArrayVertexBuffer(decalMesh.vao, 0, decalVertexBuffer.getBuffer(), 0, sizeof(DecalVertex));
        glBindVertexArray(decalMesh.vao);

        u32 tex = 0;
        u32 offset = 0;
        for (auto const& d : renderListDecal)
        {
            if (d.texture != tex)
            {
                tex = d.texture;
                glBindTextureUnit(0, d.texture);
            }
            glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(d.worldTransform));
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d.worldTransform));
            glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glUniform3f(2, d.color.x, d.color.y, d.color.z);
            glDrawArrays(GL_TRIANGLES, offset, d.count);
            offset += d.count;
        }
        renderListDecal.clear();
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // ribbons
    if (renderListRibbon.size() > 0)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, -1000.f);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glUseProgram(getShaderProgram("ribbon"));

        glVertexArrayVertexBuffer(ribbonMesh.vao, 0, ribbonVertexBuffer.getBuffer(), 0, sizeof(RibbonVertex));
        glBindVertexArray(ribbonMesh.vao);

        u32 offset = 0;
        for (auto const& r : renderListRibbon)
        {
            glDrawArrays(GL_TRIANGLES, offset, r.count);
            offset += r.count;
        }

        renderListRibbon.clear();
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // billboards
    if (renderListBillboard.size() > 0)
    {
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glUseProgram(getShaderProgram("billboard"));
        glBindVertexArray(emptyVAO);

        i32 currentTexture = -1;
        for (auto const& b : renderListBillboard)
        {
            if (currentTexture != (i32)b.texture)
            {
                glBindTextureUnit(0, b.texture);
                currentTexture = (i32)b.texture;
            }
            glUniform4fv(0, 1, (GLfloat*)&b.color);
            glm::mat4 translation = glm::translate(glm::mat4(1.f), b.position);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.f), b.angle, { 0, 0, 1 });
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(translation));
            glUniform3f(2, b.scale.x, b.scale.y, b.scale.z);
            glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rotation));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        renderListBillboard.clear();
    }

    // render to back buffer
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, game.windowWidth, game.windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(getShaderProgram("post"));
    glBindTextureUnit(0, fb.mainColorTexture);
    glm::vec2 res(game.windowWidth, game.windowHeight);
    glm::mat4 fullscreenOrtho = glm::ortho(0.f, (f32)game.windowWidth, (f32)game.windowHeight, 0.f);
    ViewportLayout& layout = viewportLayout[cameras.size() - 1];
    glBindVertexArray(emptyVAO);
    for (u32 i=0; i<cameras.size(); ++i)
    {
        glm::vec2 dir = layout.offsets[i];
        dir.x = glm::sign(dir.x);
        dir.y = glm::sign(dir.y);
        glm::mat4 matrix = fullscreenOrtho *
                           glm::translate(glm::mat4(1.f), glm::vec3(layout.offsets[i] * res + dir * (f32)viewportGapPixels, 0.f)) *
                           glm::scale(glm::mat4(1.f), glm::vec3(layout.scale * res -
                                       glm::vec2(layout.scale.x < 1.f ? viewportGapPixels : 0,
                                                 layout.scale.y < 1.f ? viewportGapPixels : 0), 1.0));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
        glUniform1ui(1, i);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // 2D
    glEnable(GL_BLEND);
    if (renderListText2D.size() > 0)
    {
        glUseProgram(getShaderProgram("text2D"));
        for (auto& q : renderListText2D)
        {
            glBindTextureUnit(0, q.tex);
            glUniform4fv(0, 4, (GLfloat*)&q.points);
            glUniform4fv(4, 1, (GLfloat*)&q.color);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        renderListText2D.clear();
    }
    if (renderListTex2D.size() > 0)
    {
        glUseProgram(getShaderProgram("tex2D"));
        for (auto& q : renderListTex2D)
        {
            glBindTextureUnit(0, q.tex);
            glUniform4fv(0, 4, (GLfloat*)&q.points);
            glUniform4fv(4, 1, (GLfloat*)&q.color);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        renderListTex2D.clear();
    }

    SDL_GL_SwapWindow(game.window);

    renderList.clear();
}

u32 Renderer::loadMesh(Mesh const& meshData)
{
    // TODO: load all meshes into one big buffer? (This might improve performance)
    GLMesh mesh;
    glCreateBuffers(1, &mesh.vbo);
    glNamedBufferData(mesh.vbo, meshData.vertices.size() * sizeof(meshData.vertices[0]), meshData.vertices.data(), GL_STATIC_DRAW);

    glCreateBuffers(1, &mesh.ebo);
    glNamedBufferData(mesh.ebo, meshData.indices.size() * sizeof(meshData.indices[0]), meshData.indices.data(), GL_STATIC_DRAW);

    enum
    {
        POSITION_BIND_INDEX = 0,
        NORMAL_BIND_INDEX = 1,
        COLOR_BIND_INDEX = 2,
        TEXCOORD_BIND_INDEX = 3
    };

    glCreateVertexArrays(1, &mesh.vao);
    glVertexArrayVertexBuffer(mesh.vao, 0, mesh.vbo, 0, meshData.stride);
    glVertexArrayElementBuffer(mesh.vao, mesh.ebo);

    glEnableVertexArrayAttrib(mesh.vao, POSITION_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mesh.vao, POSITION_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, NORMAL_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(mesh.vao, NORMAL_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, COLOR_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, COLOR_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12 + 12);
    glVertexArrayAttribBinding(mesh.vao, COLOR_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, TEXCOORD_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, TEXCOORD_BIND_INDEX, 2, GL_FLOAT, GL_FALSE, 12 + 12 + 12);
    glVertexArrayAttribBinding(mesh.vao, TEXCOORD_BIND_INDEX, 0);

    mesh.numIndices = meshData.indices.size();
    loadedMeshes.push_back(mesh);
    return (u32)(loadedMeshes.size() - 1);
}

u32 Renderer::loadTexture(Texture const& texture, u8* data, size_t size)
{
    GLuint sizedFormat, baseFormat;
    switch (texture.format)
    {
        case Texture::Format::RGBA8:
            sizedFormat = GL_RGBA8;
            baseFormat = GL_RGBA;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            break;
        case Texture::Format::R8:
            sizedFormat = GL_R8;
            baseFormat = GL_RED;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            break;
    }

    u32 mipLevels = 1 + glm::floor(glm::log2((f32)glm::max(texture.width, texture.height)));

    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, mipLevels, sizedFormat, texture.width, texture.height);
    glTextureSubImage2D(tex, 0, 0, 0, texture.width, texture.height, baseFormat, GL_UNSIGNED_BYTE, data);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAX_ANISOTROPY, 8);
    glGenerateTextureMipmap(tex);
    loadedTextures.push_back({ tex });
    return loadedTextures.size() - 1;
}

void Renderer::setViewportCount(u32 viewports)
{
    if (cameras.size() != viewports)
    {
        print("Viewport count changed.\n");
        cameras.resize(viewports);
        for (auto& shader : loadedShaders)
        {
            if (shader.hasGeometryShader)
            {
                glDeleteProgram(shader.program);
                compileShader(shader.filename, shader.defines, shader);
            }
        }

        ViewportLayout& layout = viewportLayout[cameras.size() - 1];
        fb.renderWidth = game.config.resolutionX * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0);
        fb.renderHeight = game.config.resolutionY * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0);
        u32 layers = cameras.size();

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainColorTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.shadowDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
                game.config.shadowMapResolution, game.config.shadowMapResolution, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.cszTexture);
        for (u32 i=1; i<=ARRAY_SIZE(fb.cszFramebuffers); ++i)
        {
            i32 w = i32(fb.renderWidth >> (i - 1));
            i32 h = i32(fb.renderHeight >> (i - 1));
            glTexImage3D(GL_TEXTURE_2D_ARRAY, i - 1, GL_R32F, w, h, layers, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.saoTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.saoBlurTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
}

Camera& Renderer::getCamera(u32 index)
{
    return cameras[index];
}

Camera& Renderer::setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane, f32 farPlane)
{
    ViewportLayout const& layout = viewportLayout[cameras.size() - 1];
    Camera& cam = cameras[index];
    cam.position = from;
    cam.fov = layout.fov;
    cam.nearPlane = nearPlane;
    cam.farPlane = farPlane;
    cam.view = glm::lookAt(from, to, glm::vec3(0, 0, 1));
    glm::vec2 dim = glm::vec2(game.config.resolutionX, game.config.resolutionY) * layout.scale;
    cam.aspectRatio = dim.x / dim.y;
    cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, nearPlane, farPlane);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void Renderer::drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform, Material* material)
{
    drawMesh(mesh.renderHandle, worldTransform, material);
}

void Renderer::drawMesh(u32 renderHandle, glm::mat4 const& worldTransform, Material* material)
{
    if (!material)
    {
        material = &defaultMaterial;
    }
    renderList.push_back({ renderHandle, worldTransform, material });
}

void Renderer::drawMeshOverlay(u32 renderHandle, u32 viewportIndex, glm::mat4 const& worldTransform, glm::vec3 const& color)
{
    renderListOverlay.push_back({ renderHandle, viewportIndex, worldTransform, color });
}

void Renderer::addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation)
{
}

void Renderer::addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation)
{
}

void Renderer::addDirectionalLight(glm::vec3 direction, glm::vec3 color)
{
    worldInfo.sunDirection = -glm::normalize(direction);
    worldInfo.sunColor = color;
}

void Renderer::drawLine(glm::vec3 const& p1, glm::vec3 const& p2,
        glm::vec4 const& c1, glm::vec4 const& c2)
{
    debugVertices.push_back({ p1, c1 });
    debugVertices.push_back({ p2, c2 });
}

void Renderer::drawQuad2D(u32 texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1, glm::vec2 t2,
        glm::vec3 color, f32 alpha, bool colorShader)
{
    (colorShader ? renderListTex2D : renderListText2D).push_back({
        loadedTextures[texture].tex,
        { { p1, t1 }, { { p2.x, p1.y }, { t2.x, t1.y } }, { { p1.x, p2.y }, { t1.x, t2.y } }, { p2, t2 } },
        glm::vec4(color, alpha)
    });
}

void Renderer::drawBillboard(u32 texture, glm::vec3 const& position, glm::vec3 const& scale, glm::vec4 const& color, f32 angle)
{
    renderListBillboard.push_back({ position, scale, loadedTextures[texture].tex, color, angle });
}

void Renderer::drawTrack2D(std::vector<RenderTextureItem> const& staticItems,
            SmallVec<RenderTextureItem, 16> const& dynamicItems, u32 width, u32 height, glm::vec2 pos)
{
    if (track.width != width || track.height != height)
    {
        track.width = width;
        track.height = height;

        // multisample
        const u32 samples = 4;
        glGenTextures(1, &track.multisampleTex);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, track.multisampleTex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, width, height, GL_TRUE);

        glGenRenderbuffers(1, &track.multisampleDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, track.multisampleDepthBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);

        glGenFramebuffers(1, &track.multisampleFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, track.multisampleFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, track.multisampleTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, track.multisampleDepthBuffer);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // dest
        glGenTextures(1, &track.destTex);
        glBindTexture(GL_TEXTURE_2D, track.destTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenFramebuffers(1, &track.destFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, track.destFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, track.destTex, 0);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        loadedTextures.push_back({ track.destTex });
        track.texRenderHandle = loadedTextures.size() - 1;
    }

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, track.multisampleFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    glUseProgram(getShaderProgram("mesh2D"));

    for(auto& item : staticItems)
    {
        GLMesh const& mesh = loadedMeshes[item.renderHandle];
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
        glUniform3fv(1, 1, (GLfloat*)&item.color);
        glUniform1i(2, item.overwriteColor);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    for(auto& item : dynamicItems)
    {
        GLMesh const& mesh = loadedMeshes[item.renderHandle];
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
        glUniform3fv(1, 1, (GLfloat*)&item.color);
        glUniform1i(2, item.overwriteColor);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, track.multisampleFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, track.destFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec2 size(width, height);
    drawQuad2D(track.texRenderHandle, pos-size*0.5f, pos+size*0.5f, { 0.f, 0.f }, { 1.f, 1.f }, { 1, 1, 1 }, 1.f);
}

void Renderer::drawRibbon(Ribbon const& ribbon, u32 texture)
{
    u32 size = ribbon.getRequiredBufferSize();
    if (size > 0)
    {
        void* mem = ribbonVertexBuffer.map(size);
        u32 count = ribbon.writeVerts(mem);
        ribbonVertexBuffer.unmap();
        renderListRibbon.push_back({ count, texture });
    }
}

void Renderer::drawDecal(std::vector<DecalVertex> const& verts, glm::mat4 const& transform,
        u32 texture, glm::vec3 const& color)
{
    if (verts.size() > 0)
    {
        void* mem = decalVertexBuffer.map(verts.size() * sizeof(DecalVertex));
        memcpy(mem, (void*)verts.data(), verts.size() * sizeof(DecalVertex));
        decalVertexBuffer.unmap();
        renderListDecal.push_back({ transform, (u32)verts.size(), loadedTextures[texture].tex, color });
    }
}
