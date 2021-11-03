#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>

// IF MACOS
#define OPENAL_DEPRECATED
#include "al.h"
#include "OpenAL.h"

using namespace xmath;

namespace g {
namespace snd {

enum bit_depth
{
    bits8 = 0,
    bits16 = 1,
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
        unsigned frequency = 22000;
        bool looping = false;
        bool stereo = false;
        bit_depth depth = bit_depth::bits16;
    };

    using pcm_generator = std::function<std::vector<uint8_t> (const track::description&, float, float)>;

    description desc = {};
    std::vector<ALuint> handles;
    unsigned next_handle = 0;
    pcm_generator generator = nullptr;
    float last_t = 0;

    track() = default;

    track(const description& desc, const std::vector<ALuint>& handles)
    {
        this->desc = desc;
        this->handles = handles;
    }

    bool is_streaming() const { return handles.size() > 1; }

    ALuint next(float t, float dt)
    {
        if (nullptr == generator) { return handles[0]; }

        auto buf = generator(desc, last_t, t + dt * 10);
        auto out = handles[next_handle];
        alBufferData(out, g::snd::formats[desc.stereo][desc.depth], buf.data(), buf.size(), desc.frequency);
        next_handle = (next_handle + 1) % handles.size();
        last_t = t + dt * 10;

        return out;
    }
};


struct source
{
    ALuint handle;
    track* source_track;

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

    void update(float t, float dt)
    {
        if (nullptr != source_track)
        {
            ALint processed = 0;
            alGetSourcei(handle, AL_BUFFERS_PROCESSED, &processed);
            if (processed > 0)
            {
                ALuint buffers[10];
                alSourceUnqueueBuffers(handle, processed, buffers);
            }

            ALint queued = 0;
            alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
            if (queued < source_track->handles.size())
            {
                auto next = source_track->next(t, dt);
                std::cerr << "queuing: " << next << std::endl;
                alSourceQueueBuffers(handle, 1, &next);
            }

            std::cerr << "processed: " << processed << " queued: " << queued << std::endl;
        }
    }

    void position(const vec<3>& pos) { alSourcefv(handle, AL_POSITION, pos.v); }

    void velocity(const vec<3>& vel) { alSourcefv(handle, AL_VELOCITY, vel.v); }

    void direction(const vec<3>& dir) { alSourcefv(handle, AL_DIRECTION, dir.v); }

    void play()
    {
        if (nullptr != source_track && source_track->is_streaming())
        {
            auto dt = 1.f/60.f;
            for (int i = 0; i < source_track->handles.size(); i++)
            {
                auto next = source_track->next(2 * dt * i, 2 * dt);
                alSourceQueueBuffers(handle, 1, &next);
            }
        }

        alSourcePlay(handle);
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

    alBufferData(al_buf, g::snd::formats[desc.stereo][desc.depth], buf, size, desc.frequency);

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
