#include "g.snd.h"

static ALCdevice* dev;
static ALCcontext* ctx;

constexpr static ALenum formats[2][g::snd::bit_depth::count] = {
    {AL_FORMAT_MONO8, AL_FORMAT_MONO16},
    {AL_FORMAT_STEREO8, AL_FORMAT_STEREO16},
};


void g::snd::initialize()
{
    // ensure that the audio api has been initialized
    if (nullptr == ctx)
    {
        dev = alcOpenDevice(nullptr);

        if (!dev)
        {
            std::cerr << G_TERM_RED << "Could not open audio device" << std::endl;
            throw std::runtime_error("alcOpenDevice() returned NULL");
        }

        ctx = alcCreateContext(dev, nullptr);
        alcMakeContextCurrent(ctx);
    }
}


bool g::snd::has_sound() { return ctx != nullptr; }


g::snd::track::~track()
{
    alDeleteBuffers(handles.size(), handles.data());
    handles.clear();
}

g::snd::track& g::snd::track::operator=(track&& o)
{
    if (this != &o)
    {
        desc = o.desc;
        handles = std::exchange(o.handles, {});
        next_handle = o.next_handle;
        generator = o.generator;
        length_sec = o.length_sec;
    }

    return *this;
}

bool g::snd::track::is_streaming() const { return handles.size() > 1; }

ALuint g::snd::track::next(float& last_t)
{
    if (nullptr == generator) { return handles[0]; }

    auto buf = generator(desc, last_t, last_t + desc.buffer_seconds);
    auto out = handles[next_handle];
    alBufferData(out, formats[desc.channels - 1][desc.depth - 1], buf.data(), buf.size(), desc.frequency);
    next_handle = (next_handle + 1) % handles.size();
    last_t += desc.buffer_seconds;

    return out;
}



g::snd::source::source(track* trk)
{
    assert(nullptr != trk);
    alGenSources(1, &handle);

    source_track = trk;

    alSourcei(handle, AL_LOOPING, trk->desc.looping);
    // alSourcef(handle, AL_PITCH, 1);
    // alSourcef(handle, AL_GAIN, 10);
    // alSourcef(handle, AL_REFERENCE_DISTANCE, 15);
    if (!trk->is_streaming())
    {
        alSourcei(handle, AL_BUFFER, trk->handles[0]);
    }
}


g::snd::source::~source()
{
    if (handle)
    {
        alDeleteSources(1, &handle);
    }
}

void g::snd::source::update()
{
    if (nullptr != source_track && source_track->is_streaming())
    {
        ALint processed = 0, queued = 0;
        float playback_sec = 0;

        alGetSourcei(handle, AL_BUFFERS_PROCESSED, &processed);
        if (processed > 0)
        {
            ALuint buffers[10];
            alSourceUnqueueBuffers(handle, processed, buffers);
        }

        alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
        alGetSourcef(handle, AL_SEC_OFFSET, &playback_sec);

        if (is_stopped())
        {
            alSourceRewind(handle);
            last_t = 0;
        }
        else if (queued < (ALint)source_track->handles.size())
        {
            auto next = source_track->next(last_t);
            alSourceQueueBuffers(handle, 1, &next);
        }
    }
}

vec<3> g::snd::source::position(const vec<3>& pos)
{ 
    alSourcefv(handle, AL_POSITION, pos.v); 
    return pos; 
}

vec<3> g::snd::source::position()
{
    vec<3> pos;
    alGetSourcefv(handle, AL_POSITION, pos.v); 
    return pos; 
}

vec<3> g::snd::source::velocity(const vec<3>& vel)
{
    alSourcefv(handle, AL_VELOCITY, vel.v);
    return vel;
}

vec<3> g::snd::source::velocity()
{
    vec<3> vel;
    alGetSourcefv(handle, AL_VELOCITY, vel.v);
    return vel;
}

vec<3> g::snd::source::direction(const vec<3>& dir) 
{
    alSourcefv(handle, AL_DIRECTION, dir.v);
    return dir;
}

vec<3> g::snd::source::direction() 
{
    vec<3> dir;
    alGetSourcefv(handle, AL_DIRECTION, dir.v);
    return dir;
}

void g::snd::source::play()
{
    assert(handle > 0);

    if (nullptr != source_track && source_track->is_streaming())
    {
        auto next = source_track->next(last_t);
        alSourceQueueBuffers(handle, 1, &next);

        assert(alIsBuffer(source_track->handles[0]));
    }

    assert(alIsSource(handle));

    alSourcePlay(handle);
}

void g::snd::source::seek(float time)
{
    if (nullptr != source_track)
    {
        last_t = std::max<float>(0, std::min<float>(time, source_track->length_sec));

        ALint queued;
        ALuint dequeued[10];
        alGetSourcei(handle, AL_BUFFERS_QUEUED, &queued);
        alSourceUnqueueBuffers(handle, queued, dequeued);
        alGetSourcei(handle, AL_BUFFERS_PROCESSED, &queued);
        alSourceUnqueueBuffers(handle, queued, dequeued);
    }
}

void g::snd::source::pause() { alSourcePause(handle); }

void g::snd::source::stop() { alSourceStop(handle); }

bool g::snd::source::is_playing()
{
    ALint state = 0;
    alGetSourcei(handle, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

bool g::snd::source::is_stopped()
{
    ALint state = 0;
    alGetSourcei(handle, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

namespace g
{
namespace snd
{

void set_observer(const vec<3>& position,
                         const vec<3>& velocity,
                         const quat<>& orientation)
{
    alListenerfv(AL_POSITION, position.v);
    alListenerfv(AL_VELOCITY, velocity.v);

    auto forward = orientation.rotate({0, 0, -1});

    alListenerfv(AL_ORIENTATION, forward.v);
}

}

}
