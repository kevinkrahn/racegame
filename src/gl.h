#pragma once

#include "misc.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>

const u32 MAX_BUFFERED_FRAMES = 3;
using ShaderHandle = u32;

struct ShaderDefine
{
    const char* name = "";
    const char* value = "";
};

ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines={});

namespace RenderFlags
{
    enum
    {
        CULLING,
        WIREFRAME,
        DEPTH_OFFSET,
        NO_DEPTH_READ,
        NO_DEPTH_WRITE,
        TRANSPARENT,
        HIGHLIGHT,
    };
};
