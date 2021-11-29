#pragma once
#define XMTYPE float
#include <utility>
#include <xmath.h>
#include <g.game.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <AudioFile.h>

// IF MACOS
#define OPENAL_DEPRECATED
#include "al.h"
#include "OpenAL.h"

using namespace xmath;
using namespace g::game;

namespace g {
namespace snd {

enum bit_depth
{
    invalid = 0,
    bits8 = 1,
    bits16 = 2,
    count,
};

constexpr static ALenum formats[2][bit_depth::count] = {
    {AL_FORMAT_MONO8, AL_FORMAT_MONO16},
    {AL_FORMAT_STEREO8, AL_FORMAT_STEREO16},
};

/**
 * @brief
 */
struct track
{
    struct description
    {
        unsigned frequency = 44100;
        bool looping = false;
        unsigned channels = 1;
        bit_depth depth = bit_depth::bits16;
        float buffer_seconds = 0.1f;

        inline size_t bytes_per_second() const
        {
            return frequency * channels * depth;
        }

        inline size_t bytes_per_buffer() const
        {
            return bytes_per_second() * buffer_seconds;
        }
    };

    using pcm_generator = std::function<std::vector<uint8_t> (const track::description&, float, float)>;

    description desc = {};
    std::vector<ALuint> handles;
    unsigned next_handle = 0;
    pcm_generator generator = nullptr;
    float length_sec = 0;

    track() = default;

    track(const description& desc, const std::vector<ALuint>& handles)
    {
        this->desc = desc;
        this->handles = handles;
    }

    bool is_streaming() const { return handles.size() > 1; }

    ALuint next(float& last_t)
    {
        if (nullptr == generator) { return handles[0]; }

        auto buf = generator(desc, last_t, last_t + desc.buffer_seconds);
        auto out = handles[next_handle];
        alBufferData(out, g::snd::formats[desc.channels - 1][desc.depth - 1], buf.data(), buf.size(), desc.frequency);
        next_handle = (next_handle + 1) % handles.size();
        last_t += desc.buffer_seconds;

        return out;
    }
};


struct source : public positionable, pointable, moveable 
{
    ALuint handle = 0;
    track* source_track = nullptr;
    float last_t = 0;

    source() = default;

    source(source&& s) noexcept : handle(std::exchange(s.handle, 0)),
                                  source_track(std::exchange(s.source_track, nullptr)),
                                  last_t(std::exchange(s.last_t, 0))
                                  {}

    source(track* trk)
    {
        alGenSources(1, &handle);

        source_track = trk;

        if (!trk->is_streaming())
        {
            alSourcei(handle, AL_BUFFER, trk->handles[0]);
        }
    }

    ~source()
    {
        if (handle)
        {
            alDeleteSources(1, &handle);
        }
    }

    void update()
    {
        if (nullptr != source_track)
        {
            ALint processed = 0, queued = 0, state = 0;
            float playback_sec = 0;

            alGetSourcei(handle, AL_BUFFERS_PROCESSED, &processed);
            if (processed > 0)
            {
                ALuint buffers[10];
                alSourceUnqueueBuffers(handle, processed, buffers);
            }

            alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
            alGetSourcei(handle, AL_SOURCE_STATE, &state);
            alGetSourcef(handle, AL_SEC_OFFSET, &playback_sec);

            if (state == AL_STOPPED)
            {
                alSourceRewind(handle);
                last_t = 0;
            }
            else if (queued < source_track->handles.size())
            {
                auto next = source_track->next(last_t);
                alSourceQueueBuffers(handle, 1, &next);
            }
        }
    }

    vec<3> position(const vec<3>& pos)
    { 
        alSourcefv(handle, AL_POSITION, pos.v); 
        return pos; 
    }

    vec<3> position()
    {
        vec<3> pos;
        alGetSourcefv(handle, AL_POSITION, pos.v); 
        return pos; 
    }

    vec<3> velocity(const vec<3>& vel)
    {
        alSourcefv(handle, AL_VELOCITY, vel.v);
        return vel;
    }

    vec<3> velocity()
    {
        vec<3> vel;
        alGetSourcefv(handle, AL_VELOCITY, vel.v);
        return vel;
    }

    vec<3> direction(const vec<3>& dir) 
    {
        alSourcefv(handle, AL_DIRECTION, dir.v);
        return dir;
    }

    vec<3> direction() 
    {
        vec<3> dir;
        alGetSourcefv(handle, AL_DIRECTION, dir.v);
        return dir;
    }

    void play()
    {
        assert(handle > 0);
        std::cerr << handle << std::endl;

        if (nullptr != source_track && source_track->is_streaming())
        {
            std::cerr << "queuing next" << std::endl;

            auto next = source_track->next(last_t);
            alSourceQueueBuffers(handle, 1, &next);
        }

        assert(alIsBuffer(source_track->handles[0]));
        assert(alIsSource(handle));

        alSourcePlay(handle);
    }

    void seek(float time)
    {
        if (nullptr != source_track)
        {
            last_t = std::max<float>(0, std::min<float>(time, source_track->length_sec));

            ALint queued;
            alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
            alSourceUnqueueBuffers(handle, queued, nullptr);
            alGetSourcei(handle, AL_BUFFERS_PROCESSED, &queued);
            alSourceUnqueueBuffers(handle, queued,  nullptr);
        }
    }

    void pause() { alSourcePause(handle); }

    void stop() { alSourceStop(handle); }
};


struct source_ring : public positionable, pointable, moveable
{
    std::vector<source> sources;
    unsigned next = 0;

    source_ring(track* trk, unsigned source_count)
    {
        for (unsigned i = 0; i < source_count; i++)
        {
            sources.push_back(source{trk});
        }
    }

    void update()
    {
        for (auto& src : sources) { src.update(); }
    }

    vec<3> position(const vec<3>& pos) { return sources[next].position(pos); }
    vec<3> position() { return sources[next].position(); }

    vec<3> velocity(const vec<3>& vel) { return sources[next].velocity(vel); }
    vec<3> velocity() { return sources[next].velocity(); }

    vec<3> direction(const vec<3>& dir) { return sources[next].direction(dir); }
    vec<3> direction() { return sources[next].direction(); }

    source* play()
    {
        auto playing_source = &sources[next];
        playing_source->play();
        next = (next + 1) % sources.size();
        return playing_source;
    }
};


static ALCdevice* dev;
static ALCcontext* ctx;

struct track_factory
{

static void init_openal()
{
    // ensure that the audio api has been initialized
    if (nullptr == g::snd::ctx)
    {
        g::snd::dev = alcOpenDevice(nullptr);

        if (!g::snd::dev)
        {
            std::cerr << G_TERM_RED << "Could not open audio device" << std::endl;
            throw std::runtime_error("alcOpenDevice() returned NULL");
        }

        g::snd::ctx = alcCreateContext(g::snd::dev, nullptr);
        alcMakeContextCurrent(g::snd::ctx);
    }
}

static track from_pcm_buffer(void* buf, size_t size, const track::description& desc)
{
    init_openal();

    ALuint al_buf;

    alGenBuffers(1, &al_buf);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        std::cerr << G_TERM_RED << "Could not generate buffer: " << std::string(alGetString(error)) << std::endl;
        return {};
    }

    if (desc.channels > 2)
    {
        std::cerr << G_TERM_RED << "This implementation does not support " << desc.channels << " channels" << std::endl;
        return {};
    }

    alBufferData(al_buf, g::snd::formats[desc.channels - 1][desc.depth - 1], buf, size, desc.frequency);

    return { desc, std::vector<ALuint>{al_buf} };
}

static track from_generator(track::pcm_generator generator, const track::description& desc)
{
    init_openal();

    std::vector<ALuint> bufs;

    for (unsigned i = 0; i < 3; i++)
    {
        ALuint al_buf;
        alGenBuffers(1, &al_buf);
        bufs.push_back(al_buf);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << G_TERM_RED << "Could not generate buffer: " << std::string(alGetString(error)) << std::endl;
            return {};
        }
    }

    track t = { desc, bufs };

    t.generator = generator;

    return t;
}

static track from_ogg(const std::string& path)
{
    OggVorbis_File vf;
    track::description desc;

    if (ov_fopen(path.c_str(), &vf))
    {
        std::cerr << G_TERM_RED << "ov_fopen failed for: " << path << std::endl;
        return {};
    }

    vorbis_info* vi = ov_info(&vf, -1);

    desc.channels = vi->channels;
    desc.depth = bit_depth::bits16;
    desc.buffer_seconds = 0.1f;

    auto t = from_generator([=](const track::description& desc, float t_0, float t_1) {
        
        auto bytes = desc.bytes_per_buffer();
        std::vector<uint8_t> v(bytes, 0);
        int current_section = 0;
        v.reserve(bytes);

        long pos = 0;

        ov_time_seek_lap((OggVorbis_File*)&vf, t_0);
        while (pos < v.size())
        {
            long ret = ov_read((OggVorbis_File*)&vf, (char*)v.data() + pos, bytes - pos, 0, desc.depth, 1, &current_section);
            pos += ret;

            if (ret == 0) { break; }
        }

        return v;
    }, desc);

    t.length_sec = ov_time_total(&vf, 0);

    return t;
}

static track from_wav(const std::string& path)
{
    AudioFile<int16_t> wav(path);

    track::description desc;

    desc.frequency = wav.getSampleRate();
    desc.looping   = false;
    desc.channels  = wav.getNumChannels();
    desc.depth     = (bit_depth)(wav.getBitDepth() / 8);

    std::vector<int16_t> interleaved;
    for (unsigned si = 0; si < wav.samples[0].size(); si++)
    {
        for (unsigned ci = 0; ci < desc.channels; ci++)
        {
            interleaved.push_back(wav.samples[ci][si]); 
        }
    }

    return from_pcm_buffer(interleaved.data(), interleaved.size(), desc);
}

};

static void set_observer(const vec<3>& position,
                         const vec<3>& velocity,
                         const quat<>& orientation)
{
    alListenerfv(AL_POSITION, position.v);
    alListenerfv(AL_VELOCITY, velocity.v);

    auto forward = orientation.rotate({0, 0, -1});
    auto up = orientation.rotate({0, 1, 0});

    alListenerfv(AL_ORIENTATION, forward.v);
}

} // end namespace snd
} // end namespace g
