#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// IF MACOS
#define OPENAL_DEPRECATED
#include "al.h"
#include "OpenAL.h"

using namespace xmath;

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


struct source
{
    ALuint handle = 0;
    track* source_track = nullptr;
    float last_t = 0;

    source() = default;

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
        alDeleteSources(1, &handle);
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
                last_t = 0;
            }
            else if (queued < source_track->handles.size())
            {
                auto next = source_track->next(last_t);
                alSourceQueueBuffers(handle, 1, &next);
            }
        }
    }

    void position(const vec<3>& pos) { alSourcefv(handle, AL_POSITION, pos.v); }

    void velocity(const vec<3>& vel) { alSourcefv(handle, AL_VELOCITY, vel.v); }

    void direction(const vec<3>& dir) { alSourcefv(handle, AL_DIRECTION, dir.v); }

    void play()
    {
        if (nullptr != source_track && source_track->is_streaming())
        {
            auto next = source_track->next(last_t);
            alSourceQueueBuffers(handle, 1, &next);
        }

        alSourcePlay(handle);
    }

    void seek(float time)
    {
        if (nullptr != source_track)
        {
            last_t = std::max<float>(0, std::min<float>(time, source_track->length_sec));

            // ALuint buffers[10];
            ALint queued;
            alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
            alSourceUnqueueBuffers(handle, queued, nullptr);
            alGetSourcei(handle, AL_BUFFERS_PROCESSED, &queued);
            alSourceUnqueueBuffers(handle, queued,  nullptr);

            stop();
            play();
        }
    }

    void pause() { alSourcePause(handle); }

    void stop() { alSourceStop(handle); }
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

    if (desc.channels >= 2)
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
        
        auto bytes = desc.bytes_per_second();
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
