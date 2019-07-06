#pragma once

#include "gl.h"

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

    void destroy()
    {
        if (created)
        {
            glDeleteBuffers(3, buffers);
            created = false;
        }
    }

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

    GLuint getBuffer();

    void updateData(void* data, size_t range = 0)
    {
        checkBuffers();
        glNamedBufferSubData(getBuffer(), 0, range == 0 ? size : range, data);
    }

    void* map(size_t dataSize);

    void unmap()
    {
        glUnmapNamedBuffer(getBuffer());
    }
};

