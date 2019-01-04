#include "renderer.h"
#include "game.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>

class DynamicBuffer
{
private:
    size_t size;
    GLuint buffers[MAX_BUFFERED_FRAMES];
    bool created = false;

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
};

struct GLMesh
{
    GLuint vao, vbo, ebo;
    u32 numIndices;
};

struct GLShader
{
    GLuint program;
};

struct GLTexture
{
    GLuint tex;
};

struct RenderMesh
{
    u32 meshHandle;
    glm::mat4 worldTransform;
};

struct WorldInfo
{
    glm::mat4 orthoProjection;
    glm::vec3 sunDirection;
    f32 time;
    glm::vec3 sunColor;
    f32 pad;
    glm::mat4 cameras[MAX_VIEWPORTS];
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

struct TrackTexture
{
    GLuint multisampleFramebuffer, multisampleTex, multisampleDepthBuffer;
    GLuint destFramebuffer, destTex;
    u32 width = 0;
    u32 height = 0;
    u32 texRenderHandle;
} track;

std::vector<GLMesh> loadedMeshes;
std::vector<GLTexture> loadedTextures;
std::vector<RenderMesh> renderList;
std::vector<DebugVertex> debugVertices;
std::vector<Quad2D> renderListText2D;
std::vector<Quad2D> renderListTex2D;
SmallVec<Camera, MAX_VIEWPORTS> cameras = { {} };

u32 viewportGapPixels = 1;

struct Shaders
{
    GLShader lit;
    GLShader debug;
    GLShader text2D;
    GLShader tex2D;
    GLShader post;
    GLShader mesh2D;
} shaders;

DynamicBuffer worldInfoUBO(sizeof(WorldInfo));
DynamicBuffer debugVertexBuffer(sizeof(DebugVertex) * 300000);
GLMesh debugMesh;

struct Framebuffers
{
    GLuint mainFramebuffer;
    GLuint shadowFramebuffer;

    GLuint mainColorTexture;
    GLuint mainDepthTexture;

    GLuint shadowDepthTexture;

    u32 renderWidth;
    u32 renderHeight;
} fb;

void glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string_view> const& defines)
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

GLShader loadShader(const char* filename, bool useGeometryShader=false, SmallVec<std::string_view> defines={})
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
        glGetShaderInfoLog(vertexShader, errorMessageLength, 0, errorMessage.data());
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
        glGetShaderInfoLog(fragmentShader, errorMessageLength, 0, errorMessage.data());
        error("Fragment Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
    }
    glAttachShader(program, fragmentShader);

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    if (useGeometryShader)
    {
        glShaderSources(geometryShader, shaderStr, defines.concat({ "GEOM" }));
        glCompileShader(geometryShader);
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
            std::string errorMessage(errorMessageLength, ' ');
            glGetShaderInfoLog(geometryShader, errorMessageLength, 0, errorMessage.data());
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
        glGetProgramInfoLog(program, errorMessageLength, 0, errorMessage.data());
        error("Shader Link Error: (", filename, ")\n", errorMessage, '\n');
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    return { program };
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
    //assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
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
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

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

    //glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaders.lit = loadShader("shaders/shader.glsl", true);
    shaders.debug = loadShader("shaders/debug.glsl");
    shaders.tex2D = loadShader("shaders/quad2D.glsl", false, { "COLOR" });
    shaders.text2D = loadShader("shaders/quad2D.glsl");
    shaders.post = loadShader("shaders/post.glsl");
    shaders.mesh2D = loadShader("shaders/mesh2D.glsl");

    // create debug vertex buffer
    glCreateVertexArrays(1, &debugMesh.vao);

    glEnableVertexArrayAttrib(debugMesh.vao, 0);
    glVertexArrayAttribFormat(debugMesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(debugMesh.vao, 0, 0);

    glEnableVertexArrayAttrib(debugMesh.vao, 1);
    glVertexArrayAttribFormat(debugMesh.vao, 1, 4, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(debugMesh.vao, 1, 0);

    // main framebuffer
    const u32 layers = cameras.size();
    ViewportLayout& layout = viewportLayout[cameras.size() - 1];
    fb.renderWidth = game.config.resolutionX * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0);
    fb.renderHeight = game.config.resolutionY * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0);

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

    // shadow framebuffer
    /*
    glGenTextures(1, &fb.shadowDepthTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fb.shadowDepthTexture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, game.config.shadowMapResolution, game.config.shadowMapResolution, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &fb.shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.shadowDepthTexture, 0);
    */

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return window;
}

void Renderer::render(f32 deltaTime)
{
    // update worldinfo uniform buffer
    worldInfo.time = (f32)getTime();
    worldInfo.orthoProjection = glm::ortho(0.f, (f32)game.windowWidth, (f32)game.windowHeight, 0.f);
    u32 viewportCount = cameras.size();
    for (u32 i=0; i<viewportCount; ++i)
    {
        worldInfo.cameras[i] = cameras[i].viewProjection;
    }
    worldInfoUBO.updateData(&worldInfo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBO.getBuffer());

    // 3D

    /*
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    //glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_CLAMP);

    // draw shadow map

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    */
    glViewport(0, 0, fb.renderWidth, fb.renderHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(shaders.lit.program);

    glDisable(GL_BLEND);
    Camera const& camera = cameras[0];
    for (auto const& r : renderList)
    {
        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(r.worldTransform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    glEnable(GL_BLEND);
    if (debugVertices.size() > 0)
    {
        debugVertexBuffer.updateData(debugVertices.data(), debugVertices.size() * sizeof(DebugVertex));
        glVertexArrayVertexBuffer(debugMesh.vao, 0, debugVertexBuffer.getBuffer(), 0, sizeof(DebugVertex));

        glUseProgram(shaders.debug.program);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection));

        glBindVertexArray(debugMesh.vao);
        glDrawArrays(GL_LINES, 0, debugVertices.size());
        debugVertices.clear();
    }

    // render to back buffer
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, game.windowWidth, game.windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaders.post.program);
    glBindTextureUnit(0, fb.mainColorTexture);
    glm::vec2 res(game.windowWidth, game.windowHeight);
    glm::mat4 fullscreenOrtho = glm::ortho(0.f, (f32)game.windowWidth, (f32)game.windowHeight, 0.f);
    ViewportLayout& layout = viewportLayout[cameras.size() - 1];
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
        glUseProgram(shaders.text2D.program);
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
        glUseProgram(shaders.tex2D.program);
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
        POSITION_BIND_INDEX,
        NORMAL_BIND_INDEX,
        COLOR_BIND_INDEX,
        TEXCOORD_BIND_INDEX
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
    glVertexArrayAttribFormat(mesh.vao, COLOR_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 24);
    glVertexArrayAttribBinding(mesh.vao, COLOR_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, TEXCOORD_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, TEXCOORD_BIND_INDEX, 2, GL_FLOAT, GL_FALSE, 32);
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

    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, sizedFormat, texture.width, texture.height);
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
        shaders.lit = loadShader("shaders/shader.glsl", true);

        ViewportLayout& layout = viewportLayout[cameras.size() - 1];
        fb.renderWidth = game.config.resolutionX * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0);
        fb.renderHeight = game.config.resolutionY * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0);
        u32 layers = cameras.size();
        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainColorTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, fb.renderWidth, fb.renderHeight, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    }
}

Camera& Renderer::setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 near, f32 far)
{
    ViewportLayout const& layout = viewportLayout[cameras.size() - 1];
    Camera& cam = cameras[index];
    cam.position = from;
    cam.fov = layout.fov;
    cam.near = near;
    cam.far = far;
    cam.view = glm::lookAt(from, to, glm::vec3(0, 0, 1));
    glm::vec2 dim = glm::vec2(game.config.resolutionX, game.config.resolutionY) * layout.scale;
    cam.aspectRatio = dim.x / dim.y;
    cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, near, far);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void Renderer::drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform)
{
    drawMesh(mesh.renderHandle, worldTransform);
}

void Renderer::drawMesh(u32 renderHandle, glm::mat4 const& worldTransform)
{
    renderList.push_back({ renderHandle, worldTransform });
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

    glUseProgram(shaders.mesh2D.program);

    for(auto& item : staticItems)
    {
        GLMesh const& mesh = loadedMeshes[item.renderHandle];
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
        glUniform3fv(1, 1, (GLfloat*)&item.color);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    for(auto& item : dynamicItems)
    {
        GLMesh const& mesh = loadedMeshes[item.renderHandle];
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
        glUniform3fv(1, 1, (GLfloat*)&item.color);
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
