#include "renderer.h"
#include "game.h"
#include "scene.h"

#include <stb_include.h>

constexpr u32 viewportGapPixels = 1;
constexpr GLuint colorFormat = GL_R11F_G11F_B10F;

ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines, u32 renderFlags, f32 depthOffset)
{
    return g_game.renderer->getShaderHandle(name, defines, renderFlags, depthOffset);
}

void glShaderSources(GLuint shader, const char* src,
        SmallArray<ShaderDefine> const& defines, ShaderDefine const& stageDefine)
{
    StrBuf buf;
    buf.write("#version 450\n");
    buf.writef("#define MAX_VIEWPORTS %u\n", MAX_VIEWPORTS);
    buf.writef("#define SHADOWS_ENABLED %u\n", u32(g_game.config.graphics.shadowsEnabled));
    buf.writef("#define SSAO_ENABLED %u\n", u32(g_game.config.graphics.ssaoEnabled));
    buf.writef("#define BLOOM_ENABLED %u\n", u32(g_game.config.graphics.bloomEnabled));
    buf.writef("#define SHARPEN_ENABLED %u\n", u32(g_game.config.graphics.sharpenEnabled));
    buf.writef("#define POINT_LIGHTS_ENABLED %u\n", u32(g_game.config.graphics.pointLightsEnabled));
    buf.writef("#define MOTION_BLUR_ENABLED %u\n", u32(g_game.config.graphics.motionBlurEnabled));
    buf.writef("#define FOG_ENABLED %u\n", u32(g_game.config.graphics.fogEnabled));
    buf.writef("#define MAX_POINT_LIGHTS %u\n", MAX_POINT_LIGHTS);
    buf.writef("#define LIGHT_SPLITS %u\n", LIGHT_SPLITS);
    for (auto const& d : defines)
    {
        buf.writef("#define %s %s\n", d.name, d.value);
    }
    buf.writef("#define %s %s\n", stageDefine.name, stageDefine.value);
    const char* sources[] = { buf.data(), src };
    //println("SHADER ===========================");
    //println("%s%s", buf.data(), src);
    glShaderSource(shader, 2, sources, 0);
}

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
    ShaderProgramSource const& d = shaderProgramSources[handle];
    char* filename = tmpStr("shaders/%s.glsl", d.name);

    // TODO: remove dependency on stb_include
    char errorMsg[256];
    char* shaderStr = stb_include_file(filename, (char*)"", (char*)"shaders", errorMsg);
    if (!shaderStr)
    {
        error("Shader parse error: %s", errorMsg);
    }

    GLint success, errorMessageLength;
    GLuint program = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSources(vertexShader, shaderStr, d.defines, {"VERT", ""});
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        char* errorMessage = g_tmpMem.bump<char>(errorMessageLength);
        glGetShaderInfoLog(vertexShader, errorMessageLength, 0, errorMessage);
        FATAL_ERROR("Vertex Shader Compilation Error: (%s) %s", filename, errorMessage);
    }
    glAttachShader(program, vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSources(fragmentShader, shaderStr, d.defines, {"FRAG", ""});
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        char* errorMessage = g_tmpMem.bump<char>(errorMessageLength);
        glGetShaderInfoLog(fragmentShader, errorMessageLength, 0, errorMessage);
        FATAL_ERROR("Fragment Shader Compilation Error: (%s) %s", filename, errorMessage);
    }
    glAttachShader(program, fragmentShader);

#if 0
    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    bool hasGeometryShader = shaderStr.find("GEOM");
    if (hasGeometryShader)
    {
        glShaderSources(geometryShader, shaderStr, defines, {"GEOM", ""});
        glCompileShader(geometryShader);
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
            char* errorMessage = g_tmpMem.bump<char>(errorMessageLength);
            glGetShaderInfoLog(geometryShader, errorMessageLength, 0, errorMessage);
            FATAL_ERROR("Geometry Shader Compilation Error: (%s)\n%s", filename, errorMessage);
        }
        glAttachShader(program, geometryShader);
    }
#endif

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errorMessageLength);
        char* errorMessage = g_tmpMem.bump<char>(errorMessageLength);
        glGetProgramInfoLog(program, errorMessageLength, 0, errorMessage);
        FATAL_ERROR("Shader Link Error: (%s) %s", filename, errorMessage);
    }

    free(shaderStr);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
#if 0
    glDeleteShader(geometryShader);
#endif
    if (shaderPrograms[handle].program != 0)
    {
        glDeleteProgram(shaderPrograms[handle].program);
    }
    //glObjectLabel(GL_TEXTURE, program, strlen(filename), filename);
    shaderPrograms[handle].program = program;
}

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
    loadShader("post");
    loadShader("csz");
    loadShader("csz_minify");
    loadShader("sao");
    loadShader("sao_blur");
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
    bool isMenuHidden = (g_game.currentScene
            && g_game.currentScene->isRaceInProgress && !g_game.currentScene->isPaused);
    if (!isMenuHidden)
    {
	    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Draw to Fullscreen Framebuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, fsfb.fullscreenFramebuffer);
        glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(getShaderProgram("post"));
        Vec2 res(g_game.windowWidth, g_game.windowHeight);
        Mat4 fullscreenOrtho = Mat4::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
        ViewportLayout& layout = viewportLayout[renderWorld.cameras.size() - 1];
        glBindVertexArray(emptyVAO);
        for (u32 i=0; i<renderWorld.cameras.size(); ++i)
        {
            Vec2 dir = layout.offsets[i];
            dir.x = sign(dir.x);
            dir.y = sign(dir.y);
            Mat4 matrix = fullscreenOrtho *
                Mat4::translation(Vec3(layout.offsets[i] * res + dir * (f32)viewportGapPixels, 0.f)) *
                Mat4::scaling(Vec3(layout.scale * res -
                        Vec2(layout.scale.x < 1.f ? viewportGapPixels : 0,
                                layout.scale.y < 1.f ? viewportGapPixels : 0), 1.0));
            glUniformMatrix4fv(0, 1, GL_FALSE, matrix.valuePtr());
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

        Vec2 invResolution = 1.f /
            (Vec2(g_game.windowWidth/fullscreenBlurDivisor,
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
    }
    else
    {
	    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Draw to Backbuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, g_game.windowWidth, g_game.windowHeight);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(getShaderProgram("post"));
        Vec2 res(g_game.windowWidth, g_game.windowHeight);
        Mat4 fullscreenOrtho = Mat4::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
        ViewportLayout& layout = viewportLayout[renderWorld.cameras.size() - 1];
        glBindVertexArray(emptyVAO);
        for (u32 i=0; i<renderWorld.cameras.size(); ++i)
        {
            Vec2 dir = layout.offsets[i];
            dir.x = sign(dir.x);
            dir.y = sign(dir.y);
            Mat4 matrix = fullscreenOrtho *
                Mat4::translation(Vec3(layout.offsets[i] * res + dir * (f32)viewportGapPixels, 0.f)) *
                Mat4::scaling(Vec3(layout.scale * res -
                    Vec2(layout.scale.x < 1.f ? viewportGapPixels : 0,
                            layout.scale.y < 1.f ? viewportGapPixels : 0), 1.0));
            glUniformMatrix4fv(0, 1, GL_FALSE, matrix.valuePtr());
            glBindTextureUnit(0, renderWorld.getTexture(i)->handle);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "2D Pass");
    glEnable(GL_BLEND);

    // NOTE: stable sort to preserve the order the renderables were added
    renderItems2D.stableSort([](auto& a, auto& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.shader < b.shader;
    });
    ShaderHandle previousShader = -1;
    for (auto const& renderItem : renderItems2D)
    {
        if (previousShader != renderItem.shader)
        {
            previousShader = renderItem.shader;
            ShaderProgram const& program = getShader(renderItem.shader);
            glUseProgram(program.program);
        }
        renderItem.render(renderItem.renderData);
    }

#if 0
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    for (auto& p : renderWorld.pointLights)
    {
        Vec4 tp = renderWorld.cameras[0].viewProjection * Vec4(p.position, 1.f);
        tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * g_game.windowWidth;
        tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * g_game.windowHeight;

        f32 screenSpaceLightRadius = g_game.windowHeight
            * renderWorld.cameras[0].projection[1][1] * p.radius / tp.w;

        Mat4 transform = Mat4::translation(Vec3(tp.x, tp.y, 0.f))
            * Mat4::scaling(Vec3(screenSpaceLightRadius, screenSpaceLightRadius, 0.01f));
        glUseProgram(getShaderProgram("mesh2D"));
        glUniformMatrix4fv(0, 1, GL_FALSE, fullscreenOrtho.valuePtr());
        glUniformMatrix4fv(1, 1, GL_FALSE, transform.valuePtr());
        Vec3 color(1.f);
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

    renderItems2D.clear();
    renderWorlds.clear();
    renderWorld.clear();
    tempMem.clear();
}

void RenderWorld::setViewportCount(u32 viewports)
{
    if (cameras.size() != viewports)
    {
        println("Viewport count changed from %u to %u.", cameras.size(), viewports);
        cameras.resize(viewports);
        createFramebuffers();
    }
}

Camera& RenderWorld::setViewportCamera(u32 index, Vec3 const& from,
        Vec3 const& to, f32 nearPlane, f32 farPlane, f32 fov)
{
    ViewportLayout const& layout = viewportLayout[cameras.size() - 1];
    Camera& cam = cameras[index];
    cam.position = from;
    cam.fov = fov > 0.f ? fov : layout.fov;
    cam.nearPlane = nearPlane;
    cam.farPlane = farPlane;
    cam.view = Mat4::lookAt(from, to, Vec3(0, 0, 1));
    Vec2 dim = Vec2(width, height) * layout.scale;
    cam.aspectRatio = dim.x / dim.y;
    cam.projection = Mat4::perspective(radians(cam.fov), cam.aspectRatio, nearPlane, farPlane);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void RenderWorld::addDirectionalLight(Vec3 const& direction, Vec3 const& color)
{
    worldInfo.sunDirection = -normalize(direction);
    worldInfo.sunColor = color;
}

void RenderWorld::addPointLight(Vec3 const& position, Vec3 const& color, f32 radius, f32 falloff)
{
    PointLight pointLight;
    pointLight.position = position;
    pointLight.radius = radius;
    pointLight.color = color;
    pointLight.falloff = falloff;
    pointLights.push_back(pointLight);
}

void RenderWorld::setMotionBlur(u32 viewportIndex, Vec2 const& motionBlur)
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
            Vec4 borderColor(1.f);
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
                fb.bloomBufferSize.push_back({ (i32)fb.renderWidth/(i32)i, (i32)fb.renderHeight/(i32)i });
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
    clearRenderItems(renderItems.highlightPass);
    clearRenderItems(renderItems.pickPass);
    renderItems.transparentPass.clear();
    renderItems.overlayPass.clear();
}

void RenderWorld::setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow, u32 cameraIndex)
{
    Vec3 inverseLightDir = worldInfo.sunDirection;
    Mat4 depthView = Mat4::lookAt(inverseLightDir, Vec3(0), Vec3(0, 0, 1));
    if (!g_game.config.graphics.shadowsEnabled)
    {
        worldInfo.shadowViewProjectionBias = Mat4(0.f);
        return;
    }

    if (!hasCustomShadowBounds)
    {
	    // TODO: adjust znear and zfar for better shadow quality
        Camera const& cam = cameras[cameraIndex];
        Mat4 inverseViewProj = depthView * inverse(cam.viewProjection);
        shadowBounds = computeCameraFrustumBoundingBox(inverseViewProj);
    }

    Vec3 center = (shadowBounds.min + shadowBounds.max) * 0.5f;
    f32 extent = max(
            shadowBounds.max.x-shadowBounds.min.x,
            shadowBounds.max.y-shadowBounds.min.y) * 0.5f;
    f32 snapMultiple = 2.f * extent / shadowMapResolution;
    center.x = snap(center.x, snapMultiple);
    center.y = snap(center.y, snapMultiple);
    center.z = snap(center.z, snapMultiple);
    Mat4 depthProjection = Mat4::ortho(center.x-extent, center.x+extent,
                                        center.y+extent, center.y-extent,
                                        -shadowBounds.max.z, -shadowBounds.min.z);
    Mat4 viewProj = depthProjection * depthView;

    worldInfoShadow.cameraViewProjection = viewProj;
    f32 shadowMatrix[] = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    };
    worldInfo.shadowViewProjectionBias = Mat4(shadowMatrix) * viewProj;
}

void RenderWorld::render(Renderer* renderer, f32 deltaTime)
{
    auto comparator = [](TransparentRenderItem const& a, TransparentRenderItem const& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.shader < b.shader;
    };
    renderItems.transparentPass.sort(comparator);
    renderItems.overlayPass.sort(comparator);
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
                Vec4 tp = cameras[viewportIndex].viewProjection * Vec4(p.position, 1.f);
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
    worldInfo.orthoProjection = Mat4::ortho(0.f, (f32)g_game.windowWidth, (f32)g_game.windowHeight, 0.f);
    worldInfo.cameraViewProjection = cameras[index].viewProjection;
    worldInfo.cameraProjection = cameras[index].projection;
    worldInfo.cameraView = cameras[index].view;
    worldInfo.cameraPosition = Vec4(cameras[index].position, 1.0);
    worldInfo.invResolution = 1.f / Vec2(fbs[index].renderWidth, fbs[index].renderHeight);

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1,
	        tmpStr("Render World: %s, Viewport #%u", name, index + 1));
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
            GLuint cszProgram = renderer->getShaderProgram("csz");
            glUseProgram(cszProgram);
            Camera const& cam = cameras[index];
            Vec4 clipInfo = { cam.nearPlane * cam.farPlane, cam.nearPlane - cam.farPlane, cam.farPlane, 0.f };
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

            Vec4 projInfo = {
                -2.f / (fb.renderWidth * cam.projection[0][0]),
                -2.f / (fb.renderHeight * cam.projection[1][1]),
                (1.f - cam.projection[0][2]) / cam.projection[0][0],
                (1.f + cam.projection[1][2]) / cam.projection[1][1]
            };
            const float scale = absolute(2.f * tanf(cam.fov * 0.5f));
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
    ShaderHandle previousShader = -1;
    for (auto& renderItem : renderItems.transparentPass)
    {
        if (previousShader != renderItem.shader)
        {
            previousShader = renderItem.shader;
            ShaderProgram const& program = renderer->getShader(renderItem.shader);
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
        }
        renderItem.render(renderItem.renderData);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glStencilMask(0xFF);

    // highlight selected objects in editor
    if (isEditorActive)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
	    glDepthMask(GL_FALSE);
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

        glDepthFunc(GL_LESS);
	    glDepthMask(GL_TRUE);

        // clear depth but not stencil
        glClear(GL_DEPTH_BUFFER_BIT);

        // draw into the stencil buffer when the existing stencil value is equal, cutting off the
        // parts of the object that were hidden
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
    glEnable(GL_POLYGON_OFFSET_FILL);
    previousShader = -1;
    for (auto& renderItem : renderItems.overlayPass)
    {
        if (previousShader != renderItem.shader)
        {
            previousShader = renderItem.shader;
            ShaderProgram const& program = renderer->getShader(renderItem.shader);
            glUseProgram(program.program);
            glPolygonOffset(0.f, program.depthOffset);
        }
        renderItem.render(renderItem.renderData);
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glStencilMask(0xFF);

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
            Vec2 invResolution = 1.f / (Vec2(fb.bloomBufferSize[i].x, fb.bloomBufferSize[i].y));

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
