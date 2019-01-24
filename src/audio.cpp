#include "audio.h"
#include "math.h"
#include <SDL2/SDL.h>

void SDLAudioCallback(void* userdata, u8* buf, i32 len)
{
    ((Audio*)userdata)->audioCallback(buf, len);
}

static f32 sampleAudio(Sound* sound, f32 position, u32 channel, bool looping)
{
    u32 sampleIndex1 = (u32)position       * sound->numChannels + channel;
    u32 sampleIndex2 = (u32)(position + 1) * sound->numChannels + channel;
    if (sampleIndex2 >= sound->numSamples*2)
    {
        if (looping)
        {
            sampleIndex2 -= sound->numSamples * sound->numChannels;
        }
        else
        {
            sampleIndex2 = sampleIndex1;
        }
    }

    f32 sample1 = (f32)sound->audioData[sampleIndex1] / 32767.0f;
    f32 sample2 = (f32)sound->audioData[sampleIndex2] / 32767.0f;

    f64 intpart;
    f32 result = glm::lerp(sample1, sample2, (f32)glm::abs(modf(position, &intpart)));

    return result;
}

void Audio::audioCallback(u8* buf, i32 len)
{
    {
        std::lock_guard<std::mutex> lock(audioMutex);

        // process playback modifications
        for (auto &m : playbackModifications)
        {
            if (m.type == PlaybackModification::PLAY)
            {
                playingSounds.push_back(m.newSound);
            }
            else if (m.type == PlaybackModification::MASTER_VOLUME)
            {
                masterVolume = m.volume;
            }
            else if (m.type == PlaybackModification::MASTER_PITCH)
            {
                masterPitch = m.pitch;
            }
            else
            {
                for (PlayingSound* it = playingSounds.begin(); it != playingSounds.end();)
                {
                    if (m.handle == it->handle)
                    {
                        switch (m.type)
                        {
                            case PlaybackModification::VOLUME:
                                it->volume = m.volume;
                                break;
                            case PlaybackModification::PITCH:
                                it->pitch = m.pitch;
                                break;
                            case PlaybackModification::PAN:
                                it->pan = m.pan;
                                break;
                            case PlaybackModification::STOP:
                                playingSounds.erase(it);
                                continue;
                            case PlaybackModification::SET_PAUSED:
                                it->isPaused = m.paused;
                                break;
                            default:
                                // should never happen
                                break;
                        }
                    }
                    ++it;
                }
            }
        }
        playbackModifications.clear();
    }

    f32* buffer = (f32*)buf;
    u32 numSamples = len / (sizeof(f32) * 2);

    for (u32 i=0; i<numSamples; ++i)
    {
        f32 left = 0.0f;
        f32 right = 0.0f;

        for (PlayingSound* s = playingSounds.begin(); s != playingSounds.end();)
        {
            if (s->isPaused)
            {
                continue;
            }

            if (s->playPosition >= s->sound->numSamples)
            {
                if (s->isLooping)
                {
                    s->playPosition -= s->sound->numSamples;
                }
                else
                {
                    playingSounds.erase(s);
                }
                continue;
            }

            f32 sampleLeft, sampleRight;
            sampleLeft = sampleAudio(s->sound, s->playPosition, 0, s->isLooping);

            if (s->sound->numChannels == 1)
            {
                sampleRight = sampleLeft;
            }
            else
            {
                sampleRight = sampleAudio(s->sound, s->playPosition, 1, s->isLooping);
            }

            f32 panRight = (s->pan + 1.0f) * 0.5f;
            f32 panLeft = 1.0f - panRight;

            left  = glm::clamp(left  + sampleLeft  * s->volume * panLeft, -1.0f, 1.0f);
            right = glm::clamp(right + sampleRight * s->volume * panRight, -1.0f, 1.0f);

            s->playPosition += s->pitch;
            s++;
        }

        buffer[i*2] = left;
        buffer[i*2+1] = right;
    }
}

void Audio::init()
{
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_F32;
    spec.channels = 2;
    spec.samples = 1024;
    spec.callback = SDLAudioCallback;
    spec.userdata = this;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (!audioDevice)
    {
        FATAL_ERROR("SDL_OpenAudioDevice error: ", SDL_GetError());
    }

    SDL_PauseAudioDevice(audioDevice, 0);
}

void Audio::close()
{
    SDL_CloseAudioDevice(audioDevice);
}

void Audio::setMasterVolume(f32 volume)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::MASTER_VOLUME;
    pm.volume = volume;
    playbackModifications.push_back(pm);
}

void Audio::setMasterPitch(f32 pitch)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::MASTER_PITCH;
    pm.volume = pitch;
    playbackModifications.push_back(pm);
}

SoundHandle Audio::playSound(Sound* sound, bool loop, f32 pitch, f32 volume, f32 pan)
{
    PlayingSound ps;
    ps.sound = sound;
    ps.isLooping = loop;
    ps.pitch = pitch;
    ps.volume = volume;
    ps.pan = pan;
    ps.handle = ++nextSoundHandle;

    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PLAY;
    pm.newSound = ps;
    playbackModifications.push_back(pm);

    return ps.handle;
}

void Audio::stopSound(SoundHandle handle)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::STOP;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPitch(SoundHandle handle, f32 pitch)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PITCH;
    pm.pitch = pitch;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundVolume(SoundHandle handle, f32 volume)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::VOLUME;
    pm.volume = volume;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPan(SoundHandle handle, f32 pan)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PAN;
    pm.pan = pan;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPaused(SoundHandle handle, bool paused)
{
    std::lock_guard<std::mutex> lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::SET_PAUSED;
    pm.paused = paused;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}
