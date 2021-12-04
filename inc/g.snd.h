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

    track(track&& o) noexcept : desc(o.desc),
                                handles(std::exchange(o.handles, {})),
                                next_handle(o.next_handle),
                                generator(o.generator),
                                length_sec(o.length_sec)
    {

    }

    ~track();

    track& operator=(track&& o);

    bool is_streaming() const;

    ALuint next(float& last_t);
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

    source(track* trk);

    ~source();

    void update();

    vec<3> position(const vec<3>& pos);
    vec<3> position();

    vec<3> velocity(const vec<3>& vel);
    vec<3> velocity();

    vec<3> direction(const vec<3>& dir);
    vec<3> direction();

    void play();

    void seek(float time);

    void pause();

    void stop();

    bool is_playing();

    bool is_stopped();
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


struct track_factory
{

static track from_pcm_buffer(void* buf, size_t size, const track::description& desc);

static track from_generator(track::pcm_generator generator, const track::description& desc);

static track from_ogg(const std::string& path);

static track from_wav(const std::string& path);

};

static void set_observer(const vec<3>& position,
                         const vec<3>& velocity,
                         const quat<>& orientation)
{
    alListenerfv(AL_POSITION, position.v);
    alListenerfv(AL_VELOCITY, velocity.v);

    auto forward = orientation.rotate({0, 0, -1});

    alListenerfv(AL_ORIENTATION, forward.v);
}

} // end namespace snd
} // end namespace g
