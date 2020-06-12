#pragma once

#include "misc.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>

const u32 MAX_BUFFERED_FRAMES = 3;
using ShaderHandle = u32;

namespace RenderFlags
{
    enum
    {
        BACKFACE_CULL,
        //WIREFRAME,
        DEPTH_OFFSET,
        DEPTH_READ,
        DEPTH_WRITE,
    };
};

struct ShaderDefine
{
    const char* name = "";
    const char* value = "";
};

ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines={},
        u32 renderFlags=RenderFlags::DEPTH_READ | RenderFlags::DEPTH_WRITE);
