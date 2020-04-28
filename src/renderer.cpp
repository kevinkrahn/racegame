#include "renderer.h"
#include "game.h"
#include <fstream>
#include <sstream>
#include <map>

constexpr u32 viewportGapPixels = 1;
constexpr GLuint colorFormat = GL_R11F_G11F_B10F;

void Renderer::glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines)
{
    std::ostringstream str;
    str << "#version 450\n";
    str << "#define MAX_VIEWPORTS " << MAX_VIEWPORTS << '\n';
    str << "#define SHADOWS_ENABLED " << u32(g_game.config.graphics.shadowsEnabled) << '\n';
    str << "#define SSAO_ENABLED " << u32(g_game.config.graphics.ssaoEnabled) << '\n';
    str << "#define BLOOM_ENABLED " << u32(g_game.config.graphics.bloomEnabled) << '\n';
    for (auto const& d : defines)
    {
        str << "#define " << d << '\n';
    }
    std::string tmp = str.str();
    const char* sources[] = { tmp.c_str(), src.c_str() };
    glShaderSource(shader, 2, sources, 0);
}

void Renderer::loadShader(std::string filename, SmallVec<std::string> defines, std::string name)
{
    if (name.empty())
    {
        name = filename;
    }

    filename = "shaders/" + filename + ".glsl";

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
    // TODO: investigate using stb_include
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

        size_t slashIndex = filename.find_last_of('/');
        std::string parentPath = (slashIndex == std::string::npos) ? "" : filename.substr(0, slashIndex+1);
        std::string includePath = parentPath + includeStr;

        std::ifstream file(includePath);
        if (!file)
        {
            FATAL_ERROR("Cannot load shader include file: ", includePath, " (Included from ", filename, ")");
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
        FATAL_ERROR("Vertex Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
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
        FATAL_ERROR("Fragment Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
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
            FATAL_ERROR("Geometry Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
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
        FATAL_ERROR("Shader Link Error: (", filename, ")\n", errorMessage, '\n');
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    shaderPrograms[name] = program;
}

GLuint Renderer::getShaderProgram(const char* name) const
{
    auto it = shaderPrograms.find(name);
    if (it == shaderPrograms.end())
    {
        FATAL_ERROR("Could not find shader: ", name, '\n');
    }
    return it->second;
}

void Renderer::updateFramebuffers()
{
    renderWorld.width = g_game.config.graphics.resolutionX;
    renderWorld.height = g_game.config.graphics.resolutionY;
    renderWorld.createFramebuffers();
}

void Renderer::updateFullscreenFramebuffers()
{
    createFullscreenFramebuffers();
}

void Renderer::createFullscreenFramebuffers()
{
    if (fsfb.fullscreenTexture)
    {
        glDeleteTextures(1, &fsfb.fullscreenTexture);
    }
    if (fsfb.fullscreenFramebuffer)
    {
        glDeleteFramebuffers(1, &fsfb.fullscreenFramebuffer);
    }
    if (fsfb.fullscreenBlurFramebuffer)
    {
        glDeleteTextures(2, fsfb.fullscreenBlurTextures);
        glDeleteFramebuffers(1, &fsfb.fullscreenBlurFramebuffer);
    }

    fsfb = { 0 };

    // fullscreen framebuffer
    glGenTextures(1, &fsfb.fullscreenTexture);
    glBindTexture(GL_TEXTURE_2D, fsfb.fullscreenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, g_game.windowWidth, g_game.windowHeight,
             0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &fsfb.fullscreenFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fsfb.fullscreenFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fsfb.fullscreenTexture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // fullscreen blur framebuffers
    glGenTextures(2, fsfb.fullscreenBlurTextures);
    for (u32 n=0; n<2; ++n)
    {
        glBindTexture(GL_TEXTURE_2D, fsfb.fullscreenBlurTextures[n]);
        glTexImage2D(GL_TEXTURE_2D, 0, colorFormat,
                g_game.windowWidth/fullscreenBlurDivisor, g_game.windowHeight/fullscreenBlurDivisor,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glGenFramebuffers(1, &fsfb.fullscreenBlurFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fsfb.fullscreenBlurFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fsfb.fullscreenBlurTextures[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fsfb.fullscreenBlurTextures[1], 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

void Renderer::initShaders()
{
    for (auto& p : shaderPrograms)
    {
        glDeleteProgram(p.second);
    }
    shaderPrograms.clear();

    loadShader("bloom_filter");
    loadShader("blit");
    loadShader("blit2");
    loadShader("post_process");
    loadShader("blur", { "HBLUR" }, "hblur");
    loadShader("blur", { "VBLUR" }, "vblur");
    loadShader("blur2", { "HBLUR" }, "hblur2");
    loadShader("blur2", { "VBLUR" }, "vblur2");
    loadShader("lit");
    loadShader("lit", { "ALPHA_DISCARD" }, "lit_discard");
    loadShader("debug");
    loadShader("quad2D", { "COLOR" }, "tex2D");
    loadShader("quad2D", { "BLUR" }, "texBlur2D");
    loadShader("quad2D", {}, "text2D");
    loadShader("post");
    loadShader("mesh2D");
    loadShader("billboard", { "LIT" });
    loadShader("billboard", {}, "billboard_unlit");
    loadShader("ribbon");
    loadShader("csz");
    loadShader("csz_minify");
    loadShader("sao");
    loadShader("sao_blur");
    loadShader("overlay");
    loadShader("mesh_decal");
    loadShader("terrain");
    loadShader("track");
    loadShader("flames");
}

void Renderer::init()
{
    initShaders();

    glCreateVertexArrays(1, &emptyVAO);

    updateFramebuffers();
    updateFullscreenFramebuffers();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    renderWorld.name = "Main";
}

void Renderer::render(f32 deltaTime)
{
    renderablesCount = 0;
    for (RenderWorld* rw : renderWorlds)
    {
        if (rw->settingsVersion != settingsVersion)
        {
            rw->settingsVersion = settingsVersion;
            rw->createFramebuffers();
        }
        rw->render(this, deltaTime);
        renderablesCount += rw->renderables.size();
        rw->clear();
    }

    renderWorld.render(this, deltaTime);
    renderablesCount += renderWorld.renderables.size();
    renderWorld.clear();

    // render to fullscreen texture
#if 0
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Draw to Fullscreen Framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, fsfb.fullscreenFramebuffer);
    glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(getShaderProgram("post"));
    glm::vec2 res(g_game.windowWidth, g_game.windowHeight);
    glm::mat4 fullscreenOrtho = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
    ViewportLayout& layout = viewportLayout[renderWorld.cameras.size() - 1];
    glBindVertexArray(emptyVAO);
    for (u32 i=0; i<renderWorld.cameras.size(); ++i)
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
        glBindTextureUnit(0, renderWorld.getTexture(i)->handle);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
	glPopDebugGroup();

	// blur the fullscreen framebuffer
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Blur Fullscreen Texture");
    glViewport(0, 0,
            g_game.windowWidth/fullscreenBlurDivisor,
            g_game.windowHeight/fullscreenBlurDivisor);

    glUseProgram(getShaderProgram("blit2"));
    glBindFramebuffer(GL_FRAMEBUFFER, fsfb.fullscreenBlurFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBindTextureUnit(1, fsfb.fullscreenTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glm::vec2 invResolution = 1.f /
        (glm::vec2(g_game.windowWidth/fullscreenBlurDivisor,
                   g_game.windowHeight/fullscreenBlurDivisor));

    glUseProgram(getShaderProgram("hblur2"));
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glUniform2f(2, invResolution.x, invResolution.y);
    glBindTextureUnit(1, fsfb.fullscreenBlurTextures[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUseProgram(getShaderProgram("vblur2"));
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glUniform2f(2, invResolution.x, invResolution.y);
    glBindTextureUnit(1, fsfb.fullscreenBlurTextures[1]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // sampled by gui for blurred background
    glBindTextureUnit(0, fsfb.fullscreenBlurTextures[0]);

	glPopDebugGroup();

    // 2D
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Draw to Backbuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);

    glUseProgram(getShaderProgram("blit2"));
    glBindTextureUnit(1, fsfb.fullscreenTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
#else
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Draw to Backbuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(getShaderProgram("post"));
    glm::vec2 res(g_game.windowWidth, g_game.windowHeight);
    glm::mat4 fullscreenOrtho = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
    ViewportLayout& layout = viewportLayout[renderWorld.cameras.size() - 1];
    glBindVertexArray(emptyVAO);
    for (u32 i=0; i<renderWorld.cameras.size(); ++i)
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
        glBindTextureUnit(0, renderWorld.getTexture(i)->handle);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
#endif

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "2D Pass");
    glEnable(GL_BLEND);

    std::stable_sort(renderables2D.begin(), renderables2D.end(), [&](auto& a, auto& b) {
        return a.priority < b.priority;
    });

    for (auto const& r : renderables2D)
    {
        r.renderable->on2DPass(this);
    }
	glPopDebugGroup();
	glPopDebugGroup();

    renderables2D.clear();
    renderWorlds.clear();
}

void RenderWorld::setViewportCount(u32 viewports)
{
    if (cameras.size() != viewports)
    {
        print("Viewport count changed.\n");
        cameras.resize(viewports);
        createFramebuffers();
    }
}

Camera& RenderWorld::setViewportCamera(u32 index, glm::vec3 const& from,
        glm::vec3 const& to, f32 nearPlane, f32 farPlane, f32 fov)
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
    cam.projection = glm::perspective(glm::radians(cam.fov),
            cam.aspectRatio, nearPlane, farPlane);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void RenderWorld::addDirectionalLight(glm::vec3 direction, glm::vec3 color)
{
    worldInfo.sunDirection = -glm::normalize(direction);
    worldInfo.sunColor = color;
}

void RenderWorld::updateWorldTime(f64 time)
{
    worldInfo.time = (f32)time;
}

void RenderWorld::createFramebuffers()
{
    for (auto& b : worldInfoUBO)
    {
        b.destroy();
    }
    worldInfoUBO.clear();

    for (auto& b : worldInfoUBOShadow)
    {
        b.destroy();
    }
    worldInfoUBOShadow.clear();

    for (auto& fb : fbs)
    {
        if (fb.mainFramebuffer)
        {
            glDeleteTextures(1, &fb.mainColorTexture);
            glDeleteTextures(1, &fb.mainDepthTexture);
            glDeleteFramebuffers(1, &fb.mainFramebuffer);
        }
        if (fb.finalFramebuffer)
        {
            glDeleteTextures(1, &fb.finalColorTexture);
            glDeleteFramebuffers(1, &fb.finalFramebuffer);
        }
        if (fb.msaaResolveColorTexture)
        {
            glDeleteTextures(1, &fb.msaaResolveColorTexture);
            glDeleteTextures(1, &fb.msaaResolveDepthTexture);
            glDeleteFramebuffers(1, &fb.msaaResolveFramebuffer);
            glDeleteFramebuffers(1, &fb.msaaResolveFromFramebuffer);
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
        if (fb.bloomFramebuffers.size() > 0)
        {
            glDeleteFramebuffers(fb.bloomFramebuffers.size(), &fb.bloomFramebuffers[0]);
            glDeleteTextures(fb.bloomColorTextures.size(), &fb.bloomColorTextures[0]);
        }
    }
    fbs.clear();

    for (u32 i=0; i<cameras.size(); ++i)
    {
        Framebuffers fb = { 0 };

        ViewportLayout& layout = viewportLayout[cameras.size() - 1];
        fb.renderWidth = (u32)(width * layout.scale.x - (layout.scale.x < 1.f ? viewportGapPixels : 0));
        fb.renderHeight = (u32)(height * layout.scale.y - (layout.scale.y < 1.f ? viewportGapPixels : 0));

        glEnable(GL_FRAMEBUFFER_SRGB);
        glGenTextures(1, &fb.mainColorTexture);
        glGenTextures(1, &fb.mainDepthTexture);
        if (g_game.config.graphics.msaaLevel > 0)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb.mainColorTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, g_game.config.graphics.msaaLevel,
                    colorFormat, fb.renderWidth, fb.renderHeight, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb.mainDepthTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, g_game.config.graphics.msaaLevel,
                    GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight, GL_TRUE);

            glGenTextures(1, &fb.msaaResolveColorTexture);
            glGenTextures(1, &fb.msaaResolveDepthTexture);

            glBindTexture(GL_TEXTURE_2D, fb.msaaResolveColorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                    0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, fb.msaaResolveDepthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight,
                    0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glGenFramebuffers(1, &fb.msaaResolveFramebuffer);
            glGenFramebuffers(1, &fb.msaaResolveFromFramebuffer);

            glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                    fb.msaaResolveDepthTexture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                    fb.msaaResolveColorTexture, 0);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFromFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE,
                    fb.mainDepthTexture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                    fb.mainColorTexture, 0);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, fb.mainColorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                    0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, fb.mainDepthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, fb.renderWidth, fb.renderHeight,
                    0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        glGenFramebuffers(1, &fb.mainFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.mainDepthTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.mainColorTexture, 0);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        if (g_game.config.graphics.ssaoEnabled)
        {
            // csv framebuffers
            glGenTextures(1, &fb.cszTexture);
            glBindTexture(GL_TEXTURE_2D, fb.cszTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ARRAY_SIZE(fb.cszFramebuffers) - 1);
            for (u32 i=1; i<=ARRAY_SIZE(fb.cszFramebuffers); ++i)
            {
                i32 w = i32(fb.renderWidth >> (i - 1));
                i32 h = i32(fb.renderHeight >> (i - 1));
                glTexImage2D(GL_TEXTURE_2D, i - 1, GL_R32F, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glm::vec4 borderColor(1.f);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

            glGenFramebuffers(ARRAY_SIZE(fb.cszFramebuffers), fb.cszFramebuffers);
            for (u32 i=0; i<ARRAY_SIZE(fb.cszFramebuffers); ++i)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[i]);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.cszTexture, i);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            }

            // sao framebuffers
            glGenTextures(1, &fb.saoTexture);
            glBindTexture(GL_TEXTURE_2D, fb.saoTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

            glGenTextures(1, &fb.saoBlurTexture);
            glBindTexture(GL_TEXTURE_2D, fb.saoBlurTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, fb.renderWidth, fb.renderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor);

            glGenFramebuffers(1, &fb.saoFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.saoFramebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.saoTexture, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fb.saoBlurTexture, 0);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        // shadow framebuffer
        if (g_game.config.graphics.shadowsEnabled)
        {
            shadowMapResolution = g_game.config.graphics.shadowMapResolution;
            if (width < 128 || height < 128)
            {
                shadowMapResolution = 256;
            }

            glGenTextures(1, &fb.shadowDepthTexture);
            glBindTexture(GL_TEXTURE_2D, fb.shadowDepthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                    shadowMapResolution, shadowMapResolution,
                    0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

            glGenFramebuffers(1, &fb.shadowFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.shadowDepthTexture, 0);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        // bloom framebuffers
        bloomEnabled = g_game.config.graphics.bloomEnabled;
        if (width < 128 || height < 128)
        {
            bloomEnabled = false;
        }
        if (bloomEnabled)
        {
            for (u32 i=firstBloomDivisor; i<lastBloomDivisor; i *= 2)
            {
                GLuint tex[2];
                glGenTextures(2, tex);
                for (u32 n=0; n<2; ++n)
                {
                    glBindTexture(GL_TEXTURE_2D, tex[n]);
                    glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth/i, fb.renderHeight/i,
                            0, GL_RGB, GL_FLOAT, nullptr);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    fb.bloomColorTextures.push_back(tex[n]);
                }

                GLuint framebuffer;
                glGenFramebuffers(1, &framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex[0], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex[1], 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                fb.bloomFramebuffers.push_back(framebuffer);
                fb.bloomBufferSize.push_back({ fb.renderWidth/i, fb.renderHeight/i });
            }

            glGenTextures(1, &fb.finalColorTexture);
            glBindTexture(GL_TEXTURE_2D, fb.finalColorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                    0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenFramebuffers(1, &fb.finalFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.finalFramebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.finalColorTexture, 0);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        fbs.push_back(fb);
        worldInfoUBO.push_back(DynamicBuffer(sizeof(WorldInfo)));
        worldInfoUBOShadow.push_back(DynamicBuffer(sizeof(WorldInfo)));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Texture RenderWorld::releaseTexture(u32 cameraIndex)
{
    GLuint t = tex[cameraIndex].handle;
    Framebuffers& fb = fbs[cameraIndex];

    if (t == fbs[cameraIndex].mainColorTexture)
    {
        glGenTextures(1, &fb.mainColorTexture);
        glBindTexture(GL_TEXTURE_2D, fb.mainColorTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.mainColorTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
    else if (t == fbs[cameraIndex].msaaResolveColorTexture)
    {
        glGenTextures(1, &fb.msaaResolveColorTexture);
        glBindTexture(GL_TEXTURE_2D, fb.msaaResolveColorTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                fb.msaaResolveColorTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
    else if (t == fbs[cameraIndex].finalColorTexture)
    {
        glGenTextures(1, &fb.finalColorTexture);
        glBindTexture(GL_TEXTURE_2D, fb.finalColorTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, fb.renderWidth, fb.renderHeight,
                0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.finalFramebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.finalColorTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    return tex[cameraIndex];
}

void RenderWorld::clear()
{
    renderables.clear();
    tempRenderBuffer.clear();
}

void RenderWorld::setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow, u32 cameraIndex)
{
    glm::vec3 inverseLightDir = worldInfo.sunDirection;
    glm::mat4 depthView = glm::lookAt(inverseLightDir, glm::vec3(0), glm::vec3(0, 0, 1));
    if (!g_game.config.graphics.shadowsEnabled)
    {
        worldInfo.shadowViewProjectionBias = glm::mat4(0.f);
        return;
    }

	// TODO: adjust znear and zfar for better shadow quality
    Camera const& cam = cameras[cameraIndex];
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
    f32 snapMultiple = 2.f * extent / shadowMapResolution;
    center.x = snap(center.x, snapMultiple);
    center.y = snap(center.y, snapMultiple);
    center.z = snap(center.z, snapMultiple);

    glm::mat4 depthProjection = glm::ortho(center.x-extent, center.x+extent,
                                        center.y+extent, center.y-extent, -maxz, -minz);
    glm::mat4 viewProj = depthProjection * depthView;

    worldInfoShadow.cameraViewProjection = viewProj;
    worldInfo.shadowViewProjectionBias = glm::mat4(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    ) * viewProj;
}

void RenderWorld::render(Renderer* renderer, f32 deltaTime)
{
    std::stable_sort(renderables.begin(), renderables.end(), [&](auto& a, auto& b) {
        return a.priority < b.priority;
    });

    for (u32 i=0; i<fbs.size(); ++i)
    {
        renderer->setCurrentRenderingCameraIndex(i);
        renderViewport(renderer, i, deltaTime);
    }
}

void RenderWorld::renderViewport(Renderer* renderer, u32 index, f32 deltaTime)
{
    // update worldinfo uniform buffer
    worldInfo.orthoProjection = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
    worldInfo.cameraViewProjection = cameras[index].viewProjection;
    worldInfo.cameraProjection = cameras[index].projection;
    worldInfo.cameraView = cameras[index].view;
    worldInfo.cameraPosition = glm::vec4(cameras[index].position, 1.0);

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, tstr("Render World: ", name, ", Viewport #", index + 1));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Framebuffers const& fb = fbs[index];

    i32 prevPriority = INT32_MIN;

    // shadow map
    if (g_game.config.graphics.shadowsEnabled)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Shadow Depth Pass");

		// NOTE: this is here to silence a warning (nvidia warning 131222)
        glUseProgram(renderer->getShaderProgram("blit2"));

		WorldInfo worldInfoShadow = worldInfo;
		setShadowMatrices(worldInfo, worldInfoShadow, index);

        worldInfoUBOShadow[index].updateData(&worldInfoShadow);

        // bind worldinfo with shadow matrices
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBOShadow[index].getBuffer());

        glBindFramebuffer(GL_FRAMEBUFFER, fb.shadowFramebuffer);

        glViewport(0, 0, shadowMapResolution, shadowMapResolution);
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
                r.renderable->onShadowPassPriorityTransition(renderer);
            }
            r.renderable->onShadowPass(renderer);
            prevPriority = r.priority;
        }
        glCullFace(GL_BACK);
        glDisable(GL_DEPTH_CLAMP);
		glPopDebugGroup();
    }

    // bind real worldinfo
    worldInfoUBO[index].updateData(&worldInfo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBO[index].getBuffer());

    // depth prepass
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Depth PrePass");
    glViewport(0, 0, fb.renderWidth, fb.renderHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_POLYGON_OFFSET_FILL);
    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    prevPriority = INT32_MIN;
    for (auto const& r : renderables)
    {
        if (r.priority != prevPriority)
        {
            r.renderable->onDepthPrepassPriorityTransition(renderer);
        }
        r.renderable->onDepthPrepass(renderer);
        prevPriority = r.priority;
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glPopDebugGroup();

    // resolve multi-sample depth buffer so it can be sampled by sao shader
    if (g_game.config.graphics.msaaLevel > 0)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "MSAA Depth Resolve");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.msaaResolveFromFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.msaaResolveFramebuffer);
        glBlitFramebuffer(0, 0, fb.renderWidth, fb.renderHeight,
                        0, 0, fb.renderWidth, fb.renderHeight,
                        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
        glBindTextureUnit(1, fb.msaaResolveDepthTexture);
		glPopDebugGroup();
    }
    else
    {
        glBindTextureUnit(1, fb.mainDepthTexture);
    }

    // generate csz texture
    if (g_game.config.graphics.ssaoEnabled)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "SSAO");

        bool skip = width < 128 || height < 128;
        if (skip)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fb.saoFramebuffer);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTextureUnit(4, fb.saoTexture);
        }
        else
        {
            glBindVertexArray(emptyVAO);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[0]);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glUseProgram(renderer->getShaderProgram("csz"));
            Camera const& cam = cameras[index];
            glm::vec4 clipInfo = { cam.nearPlane * cam.farPlane, cam.nearPlane - cam.farPlane, cam.farPlane, 0.f };
            glUniform4fv(1, 1, (GLfloat*)&clipInfo);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindTextureUnit(3, fb.cszTexture);

            // minify csz texture
		    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "SSAO CSZ Minify");
            glUseProgram(renderer->getShaderProgram("csz_minify"));
            for (u32 i=1; i<ARRAY_SIZE(fb.cszFramebuffers); ++i)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fb.cszFramebuffers[i]);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glViewport(0, 0, i32(fb.renderWidth >> i), i32(fb.renderHeight >> i));
                glUniform1i(0, i-1);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
		    glPopDebugGroup();

            // scaleable ambient obscurance
		    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "SSAO Texture");
            glViewport(0, 0, fb.renderWidth, fb.renderHeight);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.saoFramebuffer);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glUseProgram(renderer->getShaderProgram("sao"));

            glm::vec4 projInfo = {
                -2.f / (fb.renderWidth * cam.projection[0][0]),
                -2.f / (fb.renderHeight * cam.projection[1][1]),
                (1.f - cam.projection[0][2]) / cam.projection[0][0],
                (1.f + cam.projection[1][2]) / cam.projection[1][1]
            };
            const float scale = glm::abs(2.f * glm::tan(cam.fov * 0.5f));
            f32 projScale = fb.renderHeight / scale;
            glUniform4fv(1, 1, (GLfloat*)&projInfo);
            glUniform1f(2, projScale);

            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindTextureUnit(4, fb.saoTexture);
		    glPopDebugGroup();

        #if 1
		    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "SSAO Blur");
            glUseProgram(renderer->getShaderProgram("sao_blur"));

            // sao hblur
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glUniform2i(0, 1, 0);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindTextureUnit(4, fb.saoBlurTexture);

            // sao vblur
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glUniform2i(0, 0, 1);
            glDrawArrays(GL_TRIANGLES, 0, 3);
		    glPopDebugGroup();
        #endif
            glBindTextureUnit(4, fb.saoTexture);
		}

		glPopDebugGroup();
    }

    // color pass
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Main Color Pass");
    glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
    glBindTextureUnit(1, g_res.getTexture("sky_cubemap")->handle);
    glBindTextureUnit(3, g_res.getTexture("cloud_shadow")->handle);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (clearColorEnabled)
    {
        glClearColor(clearColor.x, clearColor.y, clearColor.y, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glDepthFunc(GL_EQUAL);
    prevPriority = INT32_MIN;
    for (auto const& r : renderables)
    {
        if (r.priority != prevPriority)
        {
            r.renderable->onLitPassPriorityTransition(renderer);
        }
        r.renderable->onLitPass(renderer);
        prevPriority = r.priority;
    }

	glPopDebugGroup();

    // resolve multi-sample color buffer
    if (g_game.config.graphics.msaaLevel > 0)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "MSAA Color Resolve");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.msaaResolveFromFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.msaaResolveFramebuffer);
        glBlitFramebuffer(0, 0, fb.renderWidth, fb.renderHeight,
                        0, 0, fb.renderWidth, fb.renderHeight,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindTextureUnit(0, fb.msaaResolveColorTexture);
		glPopDebugGroup();
        this->tex[index].handle = fb.msaaResolveColorTexture;
    }
    else
    {
        glBindTextureUnit(0, fb.mainColorTexture);
        this->tex[index].handle = fb.mainColorTexture;
    }

    // bloom
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    if (bloomEnabled)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Bloom");

        // filter the bright parts of the color buffer
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Filter Bright Parts");
        glBindFramebuffer(GL_FRAMEBUFFER, fb.bloomFramebuffers[0]);
        glViewport(0, 0, fb.bloomBufferSize[0].x, fb.bloomBufferSize[0].y);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glUseProgram(renderer->getShaderProgram("bloom_filter"));
        glDrawArrays(GL_TRIANGLES, 0, 3);
		glPopDebugGroup();

        GLuint blit = renderer->getShaderProgram("blit");
        GLuint hblur = renderer->getShaderProgram("hblur");
        GLuint vblur = renderer->getShaderProgram("vblur");

        // downscale (skip first downscale because that one is done by bloom filter)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Downscale Bloom Textures");
        glUseProgram(blit);
        for (u32 i=1; i<fb.bloomFramebuffers.size(); ++i)
        {
            glViewport(0, 0, fb.bloomBufferSize[i].x, fb.bloomBufferSize[i].y);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.bloomFramebuffers[i]);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glBindTextureUnit(1, fb.bloomColorTextures[(i-1)*2]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
		glPopDebugGroup();

        // blur the bright parts
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Blur Bloom Textures");
        for (u32 i=0; i<fb.bloomFramebuffers.size(); ++i)
        {
            glViewport(0, 0, fb.bloomBufferSize[i].x, fb.bloomBufferSize[i].y);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.bloomFramebuffers[i]);
            glm::vec2 invResolution = 1.f / (glm::vec2(fb.bloomBufferSize[i].x, fb.bloomBufferSize[i].y));

            // horizontal blur
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glUseProgram(hblur);
            glUniform2f(2, invResolution.x, invResolution.y);
            glBindTextureUnit(1, fb.bloomColorTextures[i*2]);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // vertical blur
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glUseProgram(vblur);
            glUniform2f(2, invResolution.x, invResolution.y);
            glBindTextureUnit(1, fb.bloomColorTextures[i*2 + 1]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
		glPopDebugGroup();

        // draw to final color texture
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Post Process");
        glBindFramebuffer(GL_FRAMEBUFFER, fb.finalFramebuffer);
        glViewport(0, 0, fb.renderWidth, fb.renderHeight);
        glUseProgram(renderer->getShaderProgram("post_process"));
        for (u32 i=0; i<fb.bloomFramebuffers.size(); ++i)
        {
            glBindTextureUnit(1+i, fb.bloomColorTextures[i*2]);
        }
        glDrawArrays(GL_TRIANGLES, 0, 3);
		glPopDebugGroup();

        glBindTextureUnit(0, fb.finalColorTexture);
		glPopDebugGroup();

		this->tex[index].handle = fb.finalColorTexture;
    }

	glPopDebugGroup();
}
