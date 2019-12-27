#include <stb_vorbis.c>

int decodeVorbis(const char* filename, int* channels, int* sampleRate, short** data)
{
    return stb_vorbis_decode_filename(filename, channels, sampleRate, data);
}
