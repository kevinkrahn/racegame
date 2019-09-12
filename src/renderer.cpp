#include "renderer.h"
#include "game.h"
#include <fstream>
#include <sstream>
#include <map>

constexpr u32 viewportGapPixels = 1;

void Renderer::glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines)
{
    std::ostringstream str;
    str << "#version 450\n";
    str << "#define MAX_VIEWPORTS " << MAX_VIEWPORTS << '\n';
    str << "#define VIEWPORT_COUNT " << cameras.size() << '\n';
    str << "#define SHADOWS_ENABLED " << u32(g_game.config.shadowsEnabled) << '\n';
    str << "#define SSAO_ENABLED " << u32(g_game.config.ssaoEnabled) << '\n';
    for (auto const& d : defines)
    {
        str << "#define " << d << '\n';
    }
    std::string tmp = str.str();
    const char* sources[] = { tmp.c_str(), src.c_str() };
    glShaderSource(shader, 2, sources, 0);
}

void Renderer::compileShader(std::string const& filename, SmallVec<std::string> defines, GLShader& shader)
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
        name = filename;
    }

    if (shaderHandleMap.count(name) != 0)
    {
        return shaderHandleMap[name];
    }

    std::string fullfilename = "shaders/" + filename + ".glsl";

    GLShader shader = {};
    compileShader(fullfilename, defines, shader);
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

GLuint Renderer::getShaderProgram(const char* name) const
{
    auto it = shaderHandleMap.find(name);
    assert(it != shaderHandleMap.end());
    return loadedShaders[it->second].program;
}

void Renderer::createFramebuffers()
{
    if (fb.mainFramebuffer)
    {
        glDeleteTextures(1, &fb.mainColorTexture);
        glDeleteTextures(1, &fb.mainDepthTexture);
        glDeleteFramebuffers(1, &fb.mainFramebuffer);
    }
    if (fb.msaaResolveFramebuffersCount > 0)
    {
        glDeleteTextures(1, &fb.msaaResolveColorTexture);
        glDeleteTextures(1, &fb.msaaResolveDepthTexture);
        glDeleteFramebuffers(fb.msaaResolveFramebuffersCount, fb.msaaResolveFramebuffers);
    }
    if (fb.shadowFramebuffer)
    {
        glDeleteTextures(1, &fb.shadowDepthTexture);
        glDeleteFramebuffers(1, &fb.shadowFramebuffer);
    }
    if (fb.cszFramebuffers[0])
    {
        glDeleteTextures(1, &fb.cszTexture);
        glDeleteFramebuffers(ARRAY_SIZE(fb.cszFramebuffers), fb.cszFramebuffers);
    }
    if (fb.saoFramebuffer)
    {
        glDeleteTextures(1, &fb.saoTexture);
        glDeleteTextures(1, &fb.saoBlurTexture);
        glDeleteFramebuffers(1, &fb.saoFramebuffer);
    }

    fb = { 0 };

    ViewportLayout& layout = viewportLayout[cameras.size() - 1];
    fb.renderWidth = (u32)(width * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0));
    fb.renderHeight = (u32)(height * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0));
    u32 layers = cameras.size();

    glEnable(GL_FRAMEBUFFER_SRGB);
    glGenTextures(1, &fb.mainColorTexture);
    glGenTextures(1, &fb.mainDepthTexture);
    if (g_game.config.msaaLevel > 0)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, fb.mainColorTexture);
        glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, g_game.config.msaaLevel,
                GL_RGB32F, fb.renderWidth, fb.renderHeight, layers, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, fb.mainDepthTexture);
        glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, g_game.config.msaaLevel,
                GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight, layers, GL_TRUE);

        glGenTextures(1, &fb.msaaResolveColorTexture);
        glGenTextures(1, &fb.msaaResolveDepthTexture);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.msaaResolveColorTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F, fb.renderWidth, fb.renderHeight,
                layers, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.msaaResolveDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight,
                layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        fb.msaaResolveFramebuffersCount = layers;
        glGenFramebuffers(layers, fb.msaaResolveFramebuffers);
        for (u32 i=0; i<layers; ++i)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFramebuffers[i]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    fb.msaaResolveDepthTexture, 0, i);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    fb.msaaResolveColorTexture, 0, i);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainColorTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F, fb.renderWidth, fb.renderHeight,
                layers, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.mainDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight,
                layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glGenFramebuffers(1, &fb.mainFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.mainDepthTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.mainColorTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    if (g_game.config.ssaoEnabled)
    {
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
    }

    // shadow framebuffer
    if (g_game.config.shadowsEnabled)
    {
        glGenTextures(1, &fb.shadowDepthTexture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, fb.shadowDepthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
                g_game.config.shadowMapResolution, g_game.config.shadowMapResolution, layers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
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
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::init(u32 width, u32 height)
{
    this->width = width;
    this->height = height;

    loadShader("lit");
    loadShader("debug");
    loadShader("quad2D", { "COLOR" }, "tex2D");
    loadShader("quad2D", {}, "text2D");
    loadShader("post");
    loadShader("mesh2D");
    loadShader("billboard");
    loadShader("ribbon");
    loadShader("csz");
    loadShader("csz_minify");
    loadShader("sao");
    loadShader("sao_blur");
    loadShader("overlay");
    loadShader("mesh_decal");
    loadShader("terrain");
    loadShader("track");

    glCreateVertexArrays(1, &emptyVAO);

    createFramebuffers();
}

void Renderer::setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow)
{
    glm::vec3 inverseLightDir = worldInfo.sunDirection;
    glm::mat4 depthView = glm::lookAt(inverseLightDir, glm::vec3(0), glm::vec3(0, 0, 1));
    for (u32 i=0; i<cameras.size(); ++i)
    {
        if (!g_game.config.shadowsEnabled)
        {
            worldInfo.shadowViewProjectionBias[i] = glm::mat4(0.f);
            continue;
        }

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
        f32 snapMultiple = 2.f * extent / g_game.config.shadowMapResolution;
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::sort(renderables.begin(), renderables.end(), [&](auto& a, auto& b) {
        return a.priority < b.priority;
    });

    for (auto const& r : renderables)
    {
        r.renderable->onBeforeRender(deltaTime);
    }

    // update worldinfo uniform buffer
    worldInfo.time = (f32)g_game.currentTime;
    worldInfo.orthoProjection = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
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

    i32 prevPriority = INT32_MIN;

    // shadow map
    if (g_game.config.shadowsEnabled)
    {
        worldInfoUBOShadow.updateData(&worldInfoShadow);

        // bind worldinfo with shadow matrices
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBOShadow.getBuffer());

        glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);

        glViewport(0, 0, g_game.config.shadowMapResolution, g_game.config.shadowMapResolution);
        glDepthMask(GL_TRUE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glBindTextureUnit(2, fb.shadowDepthTexture);
        glEnable(GL_DEPTH_CLAMP);
        glDisable(GL_BLEND);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.f, 4096.f);
        glCullFace(GL_FRONT);
        prevPriority = INT32_MIN;
        for (auto const& r : renderables)
        {
            if (r.priority != prevPriority)
            {
                r.renderable->onShadowPassPriorityTransition(this);
            }
            r.renderable->onShadowPass(this);
            prevPriority = r.priority;
        }
        glCullFace(GL_BACK);
        glDisable(GL_DEPTH_CLAMP);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // bind real worldinfo
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBO.getBuffer());

    // depth prepass
    glViewport(0, 0, fb.renderWidth, fb.renderHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);

    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    prevPriority = INT32_MIN;
    for (auto const& r : renderables)
    {
        if (r.priority != prevPriority)
        {
            r.renderable->onDepthPrepassPriorityTransition(this);
        }
        r.renderable->onDepthPrepass(this);
        prevPriority = r.priority;
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // resolve multi-sample depth buffer so it can be sampled by sao shader
    if (g_game.config.msaaLevel > 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.mainFramebuffer);
        for (u32 i=0; i<fb.msaaResolveFramebuffersCount; ++i)
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.msaaResolveFramebuffers[i]);
            glBlitFramebuffer(0, 0, fb.renderWidth, fb.renderHeight,
                            0, 0, fb.renderWidth, fb.renderHeight,
                            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        }
        glBindTextureUnit(1, fb.msaaResolveDepthTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    }
    else
    {
        glBindTextureUnit(1, fb.mainDepthTexture);
    }

    // generate csz texture
    if (g_game.config.ssaoEnabled)
    {
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
        glBindTextureUnit(4, fb.saoTexture);
    }

    // color pass
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
#if 1
    glClearColor(0.15f, 0.35f, 0.9f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
    glDepthFunc(GL_EQUAL);
    prevPriority = INT32_MIN;
    for (auto const& r : renderables)
    {
        if (r.priority != prevPriority)
        {
            r.renderable->onLitPassPriorityTransition(this);
        }
        r.renderable->onLitPass(this);
        prevPriority = r.priority;
        if (r.isTemporary)
        {
            r.renderable->~Renderable();
        }
    }

    // resolve multi-sample color buffer
    if (g_game.config.msaaLevel > 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.mainFramebuffer);
        for (u32 i=0; i<fb.msaaResolveFramebuffersCount; ++i)
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.msaaResolveFramebuffers[i]);
            glBlitFramebuffer(0, 0, fb.renderWidth, fb.renderHeight,
                            0, 0, fb.renderWidth, fb.renderHeight,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        glBindTextureUnit(0, fb.msaaResolveColorTexture);
    }
    else
    {
        glBindTextureUnit(0, fb.mainColorTexture);
    }

    // render to back buffer
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(getShaderProgram("post"));
    glm::vec2 res(g_game.windowWidth, g_game.windowHeight);
    glm::mat4 fullscreenOrtho = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
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

    //std::sort(renderables2D.begin(), renderables2D.end(), [&](auto& a, auto& b) {
        //return a.priority < b.priority;
    //});

    for (auto const& r : renderables2D)
    {
        r.renderable->on2DPass(this);
        if (r.isTemporary)
        {
            r.renderable->~Renderable2D();
        }
    }

    SDL_GL_SwapWindow(g_game.window);

    renderables.clear();
    renderables2D.clear();
    tempRenderBuffer.clear();
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
        createFramebuffers();
    }
}

Camera& Renderer::setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane, f32 farPlane, f32 fov)
{
    ViewportLayout const& layout = viewportLayout[cameras.size() - 1];
    Camera& cam = cameras[index];
    cam.position = from;
    cam.fov = fov > 0.f ? fov : layout.fov;
    cam.nearPlane = nearPlane;
    cam.farPlane = farPlane;
    cam.view = glm::lookAt(from, to, glm::vec3(0, 0, 1));
    glm::vec2 dim = glm::vec2(width, height) * layout.scale;
    cam.aspectRatio = dim.x / dim.y;
    cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, nearPlane, farPlane);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void Renderer::addDirectionalLight(glm::vec3 direction, glm::vec3 color)
{
    worldInfo.sunDirection = -glm::normalize(direction);
    worldInfo.sunColor = color;
}
