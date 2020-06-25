#include "audio.h"
#include "math.h"
#include "game.h"

Sound::Sound(const char* filename)
{
    loadFromFile(filename);
}

void Sound::decodeVorbisData()
{
    i16* buf;
    i32 channels;
    i32 rate;
    i32 count = decodeVorbis(rawAudioData.data(), (i32)rawAudioData.size(), &channels, &rate, &buf);
    if (count == -1)
    {
        error("Failed to decode vorbis: %s", name.cstr);
        return;
    }
    if (rate != 44100)
    {
        error("Unsupported sample rate: %i (\"%s\")", rate, name.cstr);
        return;
    }

    numSamples = count;
    numChannels = channels;
    decodedAudioData.assign((i16*)buf, (i16*)(buf + count * numChannels));
    audioData = decodedAudioData.data();
    free(buf);
}

void Sound::loadFromFile(const char* filename)
{
    Str64 fname(filename);
    Str64 ext = fname.substr(fname.size()-4);
    if (ext == ".wav")
    {
        SDL_AudioSpec spec;
        u32 size;
        u8* wavBuffer;
        if (SDL_LoadWAV(filename, &spec, &wavBuffer, &size) == nullptr)
        {
            error("Failed to load wav file: ", filename, "(", SDL_GetError(), ")\n");
            return;
        }

        if (spec.freq != 44100)
        {
            error("Failed to load wav file: ", filename, "(Unsupported frequency)\n");
            return;
        }

        if (spec.format != AUDIO_S16)
        {
            error("Failed to load wav file: ", filename, "(Unsupported sample format)\n");
            return;
        }

        numChannels = spec.channels;
        numSamples = (size / spec.channels) / sizeof(i16);
        rawAudioData.assign(wavBuffer, wavBuffer + size);
        audioData = (i16*)rawAudioData.data();
        format = AudioFormat::RAW;
        SDL_FreeWAV(wavBuffer);
    }
    else if (ext == ".ogg")
    {
        Buffer oggData = readFileBytes(filename);
        rawAudioData.resize((u32)oggData.size);
        rawAudioData.assign(oggData.data.get(), oggData.data.get() + oggData.size);
        decodeVorbisData();
        format = AudioFormat::VORBIS;
    }
    else
    {
        error("Unsupported audio format: ", filename, '\n');
    }
}

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
    f32 result = lerp(sample1, sample2, (f32)absolute(modf(position, &intpart)));

    return result;
}

void Audio::audioCallback(u8* buf, i32 len)
{
    SmallArray<Vec3> listeners;

    bool pauseGameplaySounds;
    {
        LockGuard lock(audioMutex);

        pauseGameplaySounds = this->pauseGameplaySounds;

        // process playback modifications
        for (auto &m : playbackModifications)
        {
            if (m.type == PlaybackModification::PLAY)
            {
                playingSounds.push_back(m.newSound);
            }
            else if (m.type == PlaybackModification::STOP_GAMEPLAY_SOUNDS)
            {
                for (auto it = playingSounds.begin(); it != playingSounds.end();)
                {

                    if (it->soundType == SoundType::VEHICLE ||
                        it->soundType == SoundType::GAME_SFX)
                    {
                        it = playingSounds.erase(it);
                        continue;
                    }
                    ++it;
                }
            }
            else
            {
                for (auto it = playingSounds.begin(); it != playingSounds.end();)
                {
                    if (m.handle == it->handle)
                    {
                        switch (m.type)
                        {
                            case PlaybackModification::VOLUME:
                                it->volume = m.volume;
                                break;
                            case PlaybackModification::PITCH:
                                it->targetPitch = m.pitch;
                                break;
                            case PlaybackModification::PAN:
                                it->pan = m.pan;
                                break;
                            case PlaybackModification::POSITION:
                                it->position = m.position;
                                break;
                            case PlaybackModification::STOP:
                                it = playingSounds.erase(it);
                                continue;
                            case PlaybackModification::SET_PAUSED:
                                it->isPaused = m.paused;
                                break;
                            case PlaybackModification::PLAY:
                            case PlaybackModification::STOP_GAMEPLAY_SOUNDS:
                                // not handled here
                                break;
                        }
                    }
                    ++it;
                }
            }
        }
        playbackModifications.clear();

        for (auto& p : listenerPositions)
        {
            listeners.push_back(p);
        }
    }

    f32* buffer = (f32*)buf;
    u32 numSamples = len / (sizeof(f32) * 2);

    f32 masterVolume = g_game.config.audio.masterVolume;
    for (u32 i=0; i<numSamples; ++i)
    {
        f32 left = 0.0f;
        f32 right = 0.0f;
        f32 bufferPercent = (f32)i / ((f32)(numSamples - 1));

        for (auto s = playingSounds.begin(); s != playingSounds.end();)
        {
            if (s->isPaused)
            {
                continue;
            }

            if (pauseGameplaySounds)
            {
                if (s->soundType == SoundType::VEHICLE ||
                    s->soundType == SoundType::GAME_SFX)
                {
                    ++s;
                    continue;
                }
            }

            if (s->playPosition >= s->sound->numSamples)
            {
                if (s->isLooping)
                {
                    s->playPosition -= s->sound->numSamples;
                }
                else
                {
                    s = playingSounds.erase(s);
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

            f32 attenuation = 1.f;
            if (s->is3D)
            {
                attenuation = 0.f;
                for (Vec3 const& camPos : listeners)
                {
                    f32 distance = length(s->position - camPos);
                    attenuation = max(attenuation,
                        clamp(1.f - (distance / s->sound->falloffDistance), 0.f, 1.f));
                }
                //attenuation = powf(attenuation, 2.f);
                attenuation *= attenuation;
            }

            f32 soundTypeVolume = 1.f;
            switch (s->soundType)
            {
                case SoundType::VEHICLE:
                    soundTypeVolume = g_game.config.audio.vehicleVolume;
                    break;
                case SoundType::GAME_SFX:
                case SoundType::MENU_SFX:
                    soundTypeVolume = g_game.config.audio.sfxVolume;
                    break;
                case SoundType::MUSIC:
                    soundTypeVolume = g_game.config.audio.musicVolume;
                    break;
            }

            left  = clamp(left  + sampleLeft
                    * ((s->volume * masterVolume * s->sound->volume)
                    * (soundTypeVolume * attenuation)) * panLeft, -1.0f, 1.0f);
            right = clamp(right + sampleRight
                    * ((s->volume * masterVolume * s->sound->volume)
                    * (soundTypeVolume * attenuation)) * panRight, -1.0f, 1.0f);

            f32 pitch = lerp(s->pitch, s->targetPitch, bufferPercent);
            s->playPosition += pitch  * (f32)g_game.timeDilation;
            s++;
        }

        buffer[i*2] = left;
        buffer[i*2+1] = right;
    }

    for (auto& s : playingSounds)
    {
        s.pitch = s.targetPitch;
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

SoundHandle Audio::playSound(Sound* sound, SoundType soundType,
        bool loop, f32 pitch, f32 volume, f32 pan)
{
    PlayingSound ps;
    ps.sound = sound;
    ps.isLooping = loop;
    ps.pitch = pitch;
    ps.volume = volume;
    ps.pan = pan;
    ps.handle = ++nextSoundHandle;
    ps.soundType = soundType;

    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PLAY;
    pm.newSound = ps;
    playbackModifications.push_back(pm);

    return ps.handle;
}

SoundHandle Audio::playSound3D(Sound* sound, SoundType soundType,
        Vec3 const& position, bool loop, f32 pitch, f32 volume, f32 pan)
{
    PlayingSound ps;
    ps.sound = sound;
    ps.isLooping = loop;
    ps.pitch = pitch;
    ps.volume = volume;
    ps.pan = pan;
    ps.is3D = true;
    ps.position = position;
    ps.handle = ++nextSoundHandle;
    ps.soundType = soundType;

    if (loop)
    {
        ps.playPosition = random(randomSeries, 0.f, (f32)sound->numSamples);
    }

    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PLAY;
    pm.newSound = ps;
    playbackModifications.push_back(pm);

    return ps.handle;
}

void Audio::stopSound(SoundHandle handle)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::STOP;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPitch(SoundHandle handle, f32 pitch)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PITCH;
    pm.pitch = pitch;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundVolume(SoundHandle handle, f32 volume)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::VOLUME;
    pm.volume = volume;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPan(SoundHandle handle, f32 pan)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::PAN;
    pm.pan = pan;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPosition(SoundHandle handle, Vec3 const& position)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::POSITION;
    pm.position = position;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setSoundPaused(SoundHandle handle, bool paused)
{
    LockGuard lock(audioMutex);
    PlaybackModification pm;
    pm.type = PlaybackModification::SET_PAUSED;
    pm.paused = paused;
    pm.handle = handle;
    playbackModifications.push_back(pm);
}

void Audio::setListeners(SmallArray<Vec3>const& listeners)
{
    LockGuard lock(audioMutex);
    listenerPositions.clear();
    for (auto& p : listeners)
    {
        listenerPositions.push_back(p);
    }
}

void Audio::stopAllGameplaySounds()
{
    LockGuard lock(audioMutex);

    PlaybackModification pm;
    pm.type = PlaybackModification::STOP_GAMEPLAY_SOUNDS;
    playbackModifications.push_back(pm);
}

void Audio::setPaused(bool paused)
{
    LockGuard lock(audioMutex);
    pauseGameplaySounds = paused;
}
