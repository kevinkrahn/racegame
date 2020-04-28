#include <stb_vorbis.c>

int decodeVorbis(uint8* oggData, int len, int* channels, int* sampleRate, short** data)
{
    return stb_vorbis_decode_memory(oggData, len, channels, sampleRate, data);
}
