#include "renderer.h"
#include "game.h"
#include "scene.h"
#include <sstream>

#include <stb_include.h>

constexpr u32 viewportGapPixels = 1;
constexpr GLuint colorFormat = GL_R11F_G11F_B10F;

ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines, u32 renderFlags, f32 depthOffset)
{
    return g_game.renderer->getShaderHandle(name, defines, renderFlags, depthOffset);
}

void glShaderSources(GLuint shader, std::string const& src,
        SmallArray<ShaderDefine> const& defines, ShaderDefine const& stageDefine)
{
    std::ostringstream str;
    str << "#version 450\n";
    str << "#define MAX_VIEWPORTS " << MAX_VIEWPORTS << '\n';
    str << "#define SHADOWS_ENABLED " << u32(g_game.config.graphics.shadowsEnabled) << '\n';
    str << "#define SSAO_ENABLED " << u32(g_game.config.graphics.ssaoEnabled) << '\n';
    str << "#define BLOOM_ENABLED " << u32(g_game.config.graphics.bloomEnabled) << '\n';
    str << "#define SHARPEN_ENABLED " << u32(g_game.config.graphics.sharpenEnabled) << '\n';
    str << "#define POINT_LIGHTS_ENABLED " << u32(g_game.config.graphics.pointLightsEnabled) << '\n';
    str << "#define MOTION_BLUR_ENABLED " << u32(g_game.config.graphics.motionBlurEnabled) << '\n';
    str << "#define FOG_ENABLED " << u32(g_game.config.graphics.fogEnabled) << '\n';
    str << "#define MAX_POINT_LIGHTS " << MAX_POINT_LIGHTS << '\n';
    str << "#define LIGHT_SPLITS " << LIGHT_SPLITS << '\n';
    for (auto const& d : defines)
    {
        str << "#define " << d.name << d.value << '\n';
    }
    str << "#define " << stageDefine.name << stageDefine.value << '\n';
    std::string tmp = str.str();
    const char* sources[] = { tmp.c_str(), src.c_str() };
    glShaderSource(shader, 2, sources, 0);
}

// TODO: remove
void Renderer::loadShader(const char* filename, SmallArray<const char*> defines, const char* name)
{
    SmallArray<ShaderDefine> actualDefines;
    for (auto& name : defines)
    {
        actualDefines.push_back({ name, "" });
    }
    ShaderHandle handle = getShaderHandle(filename, actualDefines, 0, 0.f);
    shaderNameMap[name ? name : filename] = handle;
}

void Renderer::loadShader(ShaderHandle handle)
{
    if (shaderPrograms[handle].program != 0)
    {
        glDeleteProgram(shaderPrograms[handle].program);
    }

    ShaderProgramSource const& d = shaderProgramSources[handle];
    std::string filename = str("shaders/", d.name, ".glsl");

    // TODO: remove dependency on stb_include
    char errorMsg[256];
    char* shaderText = stb_include_file((char*)filename.c_str(), (char*)"", (char*)"shaders", errorMsg);
    if (!shaderText)
    {
        error(errorMsg, '\n');
    }
    std::string shaderStr = shaderText;
    free(shaderText);

    GLint success, errorMessageLength;
    GLuint program = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSources(vertexShader, shaderStr, d.defines, {"VERT", ""});
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
    glShaderSources(fragmentShader, shaderStr, d.defines, {"FRAG", ""});
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

#if 0
    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    bool hasGeometryShader = shaderStr.find("GEOM") != std::string::npos;
    if (hasGeometryShader)
    {
        glShaderSources(geometryShader, shaderStr, defines, {"GEOM", ""});
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
#endif

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
#if 0
    glDeleteShader(geometryShader);
#endif
    shaderPrograms[handle].program = program;
}

// TODO: remove
void Renderer::loadShaders()
{
    loadShader("bloom_filter");
    loadShader("blit");
    loadShader("blit2");
    loadShader("post_process");
    loadShader("post_process", { "OUTLINE_ENABLED" }, "post_process_outline");
    loadShader("post_process", { "EDITOR_OUTLINE_ENABLED" }, "post_process_outline_editor");
    loadShader("blur", { "HBLUR" }, "hblur");
    loadShader("blur", { "VBLUR" }, "vblur");
    loadShader("blur2", { "HBLUR" }, "hblur2");
    loadShader("blur2", { "VBLUR" }, "vblur2");
    loadShader("quad2D", { "COLOR" }, "tex2D");
    loadShader("quad2D", { "BLUR" }, "texBlur2D");
    loadShader("quad2D", {}, "text2D");
    loadShader("post");
    loadShader("csz");
    loadShader("csz_minify");
    loadShader("sao");
    loadShader("sao_blur");
    loadShader("highlight_id");
}

ShaderHandle Renderer::getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines,
        u32 renderFlags, f32 depthOffset)
{
    for (u32 shaderIndex = 0; shaderIndex < shaderProgramSources.size(); ++shaderIndex)
    {
        ShaderProgramSource& shaderData = shaderProgramSources[shaderIndex];
        if (shaderData.name != name
                || shaderData.defines.size() != defines.size()
                || renderFlags != shaderPrograms[shaderIndex].renderFlags
                || depthOffset != shaderPrograms[shaderIndex].depthOffset)
        {
            continue;
        }
        bool definesMatch = true;
        for (u32 i=0; i<defines.size(); ++i)
        {
            if (strcmp(shaderData.defines[i].name, defines[i].name) != 0 ||
                strcmp(shaderData.defines[i].value, defines[i].value) != 0)
            {
                definesMatch = false;
                break;
            }
        }
        if (definesMatch)
        {
            return shaderIndex;
        }
    }
    shaderPrograms.push_back({ 0, renderFlags, depthOffset });
    shaderProgramSources.push_back({ name, defines });
    ShaderHandle handle = shaderPrograms.size() - 1;
    loadShader(handle);
    return handle;
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

void Renderer::reloadShaders()
{
    for (u32 i=0; i<shaderPrograms.size(); ++i)
    {
        loadShader(i);
    }
}

void Renderer::init()
{
    loadShaders();

    glCreateVertexArrays(1, &emptyVAO);

    updateFramebuffers();
    updateFullscreenFramebuffers();

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DITHER);

    renderWorld.name = "Main";
}

void Renderer::render(f32 deltaTime)
{
    TIMED_BLOCK();

    for (RenderWorld* rw : renderWorlds)
    {
        if (rw->settingsVersion != settingsVersion)
        {
            rw->settingsVersion = settingsVersion;
            rw->createFramebuffers();
        }
        rw->render(this, deltaTime);
        rw->clear();
    }

    renderWorld.render(this, deltaTime);
    //renderWorld.clear();

    // render to fullscreen texture
#if 1
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

    // NOTE: stable sort to preserve the order the renderables were added
    renderables2D.stableSort([](auto& a, auto& b) { return a.priority < b.priority; });
    for (auto const& r : renderables2D)
    {
        r.renderable->on2DPass(this);
    }

#if 0
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    for (auto& p : renderWorld.pointLights)
    {
        glm::vec4 tp = renderWorld.cameras[0].viewProjection * glm::vec4(p.position, 1.f);
        tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * g_game.windowWidth;
        tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * g_game.windowHeight;

        f32 screenSpaceLightRadius = g_game.windowHeight
            * renderWorld.cameras[0].projection[1][1] * p.radius / tp.w;

        glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(tp.x, tp.y, 0.f))
            * glm::scale(glm::mat4(1.f), glm::vec3(screenSpaceLightRadius, screenSpaceLightRadius, 0.01f));
        glUseProgram(getShaderProgram("mesh2D"));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(fullscreenOrtho));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(transform));
        glm::vec3 color(1.f);
        glUniform3fv(2, 1, (GLfloat*)&color);
        glUniform1i(3, 0);
        glUniform1i(4, 0);
        Mesh* sphereMesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
        glBindVertexArray(sphereMesh->vao);
        glDrawElements(GL_TRIANGLES, sphereMesh->numIndices, GL_UNSIGNED_INT, 0);
    }
#endif

	glPopDebugGroup();
	glPopDebugGroup();

    renderables2D.clear();
    renderWorlds.clear();
    renderWorld.clear();
    tempMem.clear();
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

void RenderWorld::addDirectionalLight(glm::vec3 const& direction, glm::vec3 const& color)
{
    worldInfo.sunDirection = -glm::normalize(direction);
    worldInfo.sunColor = color;
}

void RenderWorld::addPointLight(glm::vec3 const& position, glm::vec3 const& color, f32 radius, f32 falloff)
{
    PointLight pointLight;
    pointLight.position = position;
    pointLight.radius = radius;
    pointLight.color = color;
    pointLight.falloff = falloff;
    pointLights.push_back(pointLight);
}

void RenderWorld::setMotionBlur(u32 viewportIndex, glm::vec2 const& motionBlur)
{
    this->motionBlur[viewportIndex] = motionBlur;
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
            glDeleteTextures(1, &fb.stencilViewTexture);
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
        if (fb.pickFramebuffer)
        {
            glDeleteTextures(1, &fb.pickIDTexture);
            glDeleteRenderbuffers(1, &fb.pickDepthTexture);
            glDeleteFramebuffers(1, &fb.pickFramebuffer);
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
        glGenTextures(1, &fb.stencilViewTexture);
        if (g_game.config.graphics.msaaLevel > 0)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb.mainColorTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, g_game.config.graphics.msaaLevel,
                    colorFormat, fb.renderWidth, fb.renderHeight, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb.mainDepthTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, g_game.config.graphics.msaaLevel,
                    GL_DEPTH24_STENCIL8, fb.renderWidth, fb.renderHeight, GL_TRUE);

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
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, fb.renderWidth, fb.renderHeight);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

            glGenFramebuffers(1, &fb.msaaResolveFramebuffer);
            glGenFramebuffers(1, &fb.msaaResolveFromFramebuffer);

            glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                    fb.msaaResolveDepthTexture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                    fb.msaaResolveColorTexture, 0);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            glBindFramebuffer(GL_FRAMEBUFFER, fb.msaaResolveFromFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                    GL_TEXTURE_2D_MULTISAMPLE, fb.mainDepthTexture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                    fb.mainColorTexture, 0);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            glTextureView(fb.stencilViewTexture, GL_TEXTURE_2D, fb.msaaResolveDepthTexture,
                    GL_DEPTH24_STENCIL8, 0, 1, 0, 1);
            glBindTexture(GL_TEXTURE_2D, fb.stencilViewTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
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
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, fb.renderWidth, fb.renderHeight);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

            glTextureView(fb.stencilViewTexture, GL_TEXTURE_2D, fb.mainDepthTexture,
                    GL_DEPTH24_STENCIL8, 0, 1, 0, 1);
            glBindTexture(GL_TEXTURE_2D, fb.stencilViewTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
        }

        glGenFramebuffers(1, &fb.mainFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.mainFramebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, fb.mainDepthTexture, 0);
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

        if (width > 128 && height > 128)
        {
            glGenTextures(1, &fb.pickIDTexture);
            glBindTexture(GL_TEXTURE_2D, fb.pickIDTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, fb.renderWidth / 2, fb.renderHeight / 2,
                    0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glGenRenderbuffers(1, &fb.pickDepthTexture);
            glBindRenderbuffer(GL_RENDERBUFFER, fb.pickDepthTexture);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                    fb.renderWidth / 2, fb.renderHeight / 2);

            glGenFramebuffers(1, &fb.pickFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fb.pickFramebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.pickIDTexture, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                    fb.pickDepthTexture);

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
    pointLights.clear();

    auto clearRenderItems = [](auto& renderItems) {
        for (auto& pair : renderItems)
        {
            pair.value.clear();
        }
    };

    clearRenderItems(renderItems.depthPrepass);
    clearRenderItems(renderItems.shadowPass);
    clearRenderItems(renderItems.opaqueColorPass);
    clearRenderItems(renderItems.transparentPass);
    clearRenderItems(renderItems.highlightPass);
    clearRenderItems(renderItems.pickPass);
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

    if (!hasCustomShadowBounds)
    {
	    // TODO: adjust znear and zfar for better shadow quality
        Camera const& cam = cameras[cameraIndex];
        glm::mat4 inverseViewProj = depthView * glm::inverse(cam.viewProjection);
        shadowBounds = computeCameraFrustumBoundingBox(inverseViewProj);
    }

    glm::vec3 center = (shadowBounds.min + shadowBounds.max) * 0.5f;
    f32 extent = glm::max(
            shadowBounds.max.x-shadowBounds.min.x,
            shadowBounds.max.y-shadowBounds.min.y) * 0.5f;
    f32 snapMultiple = 2.f * extent / shadowMapResolution;
    center.x = snap(center.x, snapMultiple);
    center.y = snap(center.y, snapMultiple);
    center.z = snap(center.z, snapMultiple);
    glm::mat4 depthProjection = glm::ortho(center.x-extent, center.x+extent,
                                        center.y+extent, center.y-extent,
                                        -shadowBounds.max.z, -shadowBounds.min.z);
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
    for (u32 i=0; i<fbs.size(); ++i)
    {
        renderer->setCurrentRenderingCameraIndex(i);
        renderViewport(renderer, i, deltaTime);
    }
}

void RenderWorld::partitionPointLights(u32 viewportIndex)
{
    f32 partitionWidth = fbs[viewportIndex].renderWidth / LIGHT_SPLITS;
    f32 partitionHeight = fbs[viewportIndex].renderHeight / LIGHT_SPLITS;
    for (u32 x = 0; x<LIGHT_SPLITS; ++x)
    {
        f32 splitX = x * partitionWidth;

        for (u32 y = 0; y<LIGHT_SPLITS; ++y)
        {
            f32 splitY = y * partitionHeight;

            // TODO: perform circle vs box collision so that each light affects fewer pixels
            u32 lightCount = 0;
            for (auto& p : pointLights)
            {
                glm::vec4 tp = cameras[viewportIndex].viewProjection * glm::vec4(p.position, 1.f);
                tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * fbs[viewportIndex].renderWidth;
                tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * fbs[viewportIndex].renderHeight;

                f32 screenSpaceLightRadius = fbs[viewportIndex].renderHeight
                    * cameras[viewportIndex].projection[1][1] * p.radius / tp.w;

                if (!(tp.x - screenSpaceLightRadius > splitX + partitionWidth
                    || tp.x + screenSpaceLightRadius < splitX
                    || tp.y - screenSpaceLightRadius > splitY + partitionHeight
                    || tp.y + screenSpaceLightRadius < splitY))
                {
                    worldInfo.lightPartitions[x][y].pointLights[lightCount++] = p;
                    if (lightCount >= MAX_POINT_LIGHTS)
                    {
                        break;
                    }
                }
            }
            worldInfo.lightPartitions[x][y].pointLightCount = lightCount;
        }
    }
}

void RenderWorld::renderViewport(Renderer* renderer, u32 index, f32 deltaTime)
{
    TIMED_BLOCK();

    // update worldinfo uniform buffer
    partitionPointLights(index);
    worldInfo.orthoProjection = glm::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
    worldInfo.cameraViewProjection = cameras[index].viewProjection;
    worldInfo.cameraProjection = cameras[index].projection;
    worldInfo.cameraView = cameras[index].view;
    worldInfo.cameraPosition = glm::vec4(cameras[index].position, 1.0);
    worldInfo.invResolution = 1.f / glm::vec2(fbs[index].renderWidth, fbs[index].renderHeight);

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, tstr("Render World: ", name, ", Viewport #", index + 1));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool isEditorActive = g_game.isEditing
        && ((g_game.currentScene && !g_game.currentScene->isRaceInProgress) || !g_game.currentScene);

    Framebuffers const& fb = fbs[index];

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

        for (auto& pair : renderItems.shadowPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                renderItem.render(renderItem.renderData);
            }
        }

	    glDisable(GL_POLYGON_OFFSET_FILL);
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
    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    for (auto& pair : renderItems.depthPrepass)
    {
        ShaderProgram const& program = renderer->getShader(pair.key);
        glUseProgram(program.program);
        for (auto& renderItem : pair.value)
        {
            renderItem.render(renderItem.renderData);
        }
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
    glStencilMask(0xFF);
    glClearStencil(0);
    glClearColor(clearColor.x, clearColor.y, clearColor.y, clearColor.w);
    GLuint clearBits = GL_STENCIL_BUFFER_BIT;
    if (clearColorEnabled)
    {
        clearBits |= GL_COLOR_BUFFER_BIT;
    }
    glClear(clearBits);
    glDisable(GL_BLEND);
    glDepthFunc(GL_EQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glEnable(GL_CULL_FACE);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glDepthMask(GL_FALSE);
    for (auto& pair : renderItems.opaqueColorPass)
    {
        ShaderProgram const& program = renderer->getShader(pair.key);
        glUseProgram(program.program);
        for (auto& renderItem : pair.value)
        {
            glStencilFunc(GL_ALWAYS, renderItem.stencil, 0xFF);
            renderItem.render(renderItem.renderData);
        }
    }
    glStencilMask(0x0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    for (auto& pair : renderItems.transparentPass)
    {
        ShaderProgram const& program = renderer->getShader(pair.key);
        glUseProgram(program.program);
        glPolygonOffset(0.f, program.depthOffset);
#if 0
        if (program.renderFlags & RenderFlags::BACKFACE_CULL)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
        if (program.renderFlags & RenderFlags::DEPTH_READ)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
#endif
        for (auto& renderItem : pair.value)
        {
            renderItem.render(renderItem.renderData);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glStencilMask(0xFF);

    /*
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_CULL_FACE);
    glStencilMask(0xFF);

	if (highlightStep == 0)
	{
	    glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
	    glDepthMask(GL_FALSE);
	}
	else if (highlightStep < 3)
	{
	    glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
	    glDepthMask(GL_TRUE);
	}


    if (highlightStep == 0)
    {
        glStencilFunc(GL_ALWAYS, stencil, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }
    else if (highlightStep == 1)
    {
        glStencilFunc(GL_EQUAL, stencil, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }
    else if (highlightStep == 2)
    {
        glStencilFunc(GL_ALWAYS, stencil | 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }
    */

    // highlight selected objects in editor
    if (isEditorActive)
    {
        glEnable(GL_DEPTH_TEST);
	    glDepthMask(GL_FALSE);
        glDepthFunc(GL_EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        // draw into the stencil buffer where the depth is equal
        for (auto& pair : renderItems.highlightPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                glStencilFunc(GL_ALWAYS, renderItem.stencil, 0xFF);
                renderItem.render(renderItem.renderData);
            }
        }

        // clear depth but not stencil
        glClear(GL_DEPTH_BUFFER_BIT);

        // draw into the stencil buffer when the existing stencil value is equal, cutting off the
        // parts of the object that were hidden
	    glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
	    glDepthMask(GL_TRUE);
        for (auto& pair : renderItems.highlightPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                glStencilFunc(GL_EQUAL, renderItem.stencil, 0xFF);
                renderItem.render(renderItem.renderData);
            }
        }

        // draw the hidden parts with the hidden flag (0x1) set
        for (auto& pair : renderItems.highlightPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                glStencilFunc(GL_ALWAYS, renderItem.stencil | 1, 0xFF);
                renderItem.render(renderItem.renderData);
            }
        }
    }
    // highlight hidden vehicles
    else
    {
        glDisable(GL_DEPTH_TEST);
	    glDepthMask(GL_FALSE);
        glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
        for (auto& pair : renderItems.highlightPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                if (renderItem.cameraIndex != index)
                {
                    continue;
                }
                glStencilFunc(GL_EQUAL, 0, 0xFF);
                renderItem.render(renderItem.renderData);
            }
        }
    }
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0x0);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glPopDebugGroup();

    // color picking
    if (isPickPixelPending)
    {
	    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Color Pick Pass");
        glViewport(0, 0, fb.renderWidth / 2, fb.renderHeight / 2);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.pickFramebuffer);
	    glDepthFunc(GL_LESS);
	    glDepthMask(GL_TRUE);
	    glEnable(GL_DEPTH_TEST);
	    glDisable(GL_CULL_FACE);
	    glDisable(GL_BLEND);
	    glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (auto& pair : renderItems.pickPass)
        {
            ShaderProgram const& program = renderer->getShader(pair.key);
            glUseProgram(program.program);
            for (auto& renderItem : pair.value)
            {
                renderItem.render(renderItem.renderData);
            }
        }
        glPopDebugGroup();
        isPickPixelPending = false;
    }
    for (auto it = pickPixelResults.begin(); it != pickPixelResults.end();)
    {
        auto& result = *it;
        if (!result.sent)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, result.pbo);
            glReadPixels((i32)(result.pickPos.x * fb.renderWidth / 2),
                         (i32)((1.f - result.pickPos.y) * fb.renderHeight / 2),
                         1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
            result.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            result.sent = true;
        }
        else if (!result.ready)
        {
            u32 syncResult = glClientWaitSync(result.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (syncResult == GL_ALREADY_SIGNALED || syncResult == GL_CONDITION_SATISFIED)
            {
                // TODO: find out why this still causes a hitch, even though the fence is signaled
                glGetNamedBufferSubData(result.pbo, 0, sizeof(u32), &result.pickID);
                result.ready = true;
                glDeleteSync(result.fence);
            }
        }
        else
        {
            pickPixelResults.erase(it);
            continue;
        }
        ++it;
    }

    // resolve multi-sample color buffer
    if (g_game.config.graphics.msaaLevel > 0)
    {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "MSAA Color Resolve");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.msaaResolveFromFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.msaaResolveFramebuffer);
        glBlitFramebuffer(0, 0, fb.renderWidth, fb.renderHeight,
                        0, 0, fb.renderWidth, fb.renderHeight,
                        GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
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
        glUseProgram(renderer->getShaderProgram(
                    isEditorActive ? "post_process_outline_editor" : "post_process_outline"));
        glUniform4fv(0, 1, (f32*)&highlightColor[index]);
        glUniform2fv(1, 1, (f32*)&motionBlur[index]);
        for (u32 i=0; i<fb.bloomFramebuffers.size(); ++i)
        {
            glBindTextureUnit(1+i, fb.bloomColorTextures[i*2]);
        }
        glBindTextureUnit(4, fb.stencilViewTexture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
		glPopDebugGroup();

        glBindTextureUnit(0, fb.finalColorTexture);
		glPopDebugGroup();

		this->tex[index].handle = fb.finalColorTexture;
    }

	glPopDebugGroup();
}
