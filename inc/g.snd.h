#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>

// IF MACOS
#include "al.h"
#include "OpenAL.h"

using namespace xmath;

namespace g {
namespace snd {

/**
 * @brief      A short sound which is loaded entirely into memory
 */
struct track
{
    ALuint handle;

    track() = default;

    enum bit_depth
    {
        bits8 = 0,
        bits16 = 1,
        count,
    };

    struct desc
    {
        unsigned frequency = 22000;
        bool looping = false;
        bool stereo = false;
        bit_depth depth = bit_depth::bits16;
    };

    bool is_streaming() const { return false; }
};

struct source
{
    ALuint handle;

    source() = default;

    source(const track& trk)
    {
        alGenSources(1, &handle);

        if (trk.is_streaming())
        {

        }
        else
        {
            alSourcei(handle, AL_BUFFER, trk.handle);
        }
    }

    ~source()
    {
        alDeleteSources(1, &handle);
    }

    void position(const vec<3>& pos) { alSourcefv(handle, AL_POSITION, pos.v); }

    void velocity(const vec<3>& vel) { alSourcefv(handle, AL_VELOCITY, vel.v); }

    void direction(const vec<3>& dir) { alSourcefv(handle, AL_DIRECTION, dir.v); }

    void play() { alSourcePlay(handle); }

    void pause() { alSourcePause(handle); }

    void stop() { alSourceStop(handle); }
};

static ALCdevice* dev;
static ALCcontext* ctx;

struct track_factory
{

static track from_pcm_buffer(void* buf, size_t size, const track::desc& desc)
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

    ALuint al_buf;

    alGenBuffers(1, &al_buf);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        std::cerr << G_TERM_RED << "Could not generate buffer: " << std::string(alGetString(error)) << std::endl;
        return {};
    }

    static ALenum formats[2][track::bit_depth::count] = {
        {AL_FORMAT_MONO8, AL_FORMAT_MONO16},
        {AL_FORMAT_STEREO8, AL_FORMAT_STEREO16},
    };

    alBufferData(al_buf, formats[desc.stereo][desc.depth], buf, size, desc.frequency);

    return { al_buf };
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
