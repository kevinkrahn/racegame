#include "dynamic_buffer.h"
#include "game.h"

GLuint DynamicBuffer::getBuffer()
{
    checkBuffers();
    return buffers[g_game.frameIndex];
}

void* DynamicBuffer::map(size_t dataSize)
{
    if (bufferIndex != g_game.frameIndex)
    {
        offset = 0;
        bufferIndex = g_game.frameIndex;
    }

    assert(offset + dataSize < size);

    void* ptr = glMapNamedBufferRange(getBuffer(), offset, dataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
    offset += dataSize;
    return ptr;
}
