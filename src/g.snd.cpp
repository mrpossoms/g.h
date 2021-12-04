#include "g.snd.h"


constexpr static ALenum formats[2][g::snd::bit_depth::count] = {
    {AL_FORMAT_MONO8, AL_FORMAT_MONO16},
    {AL_FORMAT_STEREO8, AL_FORMAT_STEREO16},
};


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
    alGenSources(1, &handle);

    source_track = trk;

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

        if (is_stopped())
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
    std::cerr << handle << std::endl;

    if (nullptr != source_track && source_track->is_streaming())
    {
        auto next = source_track->next(last_t);
        alSourceQueueBuffers(handle, 1, &next);
    }

    assert(alIsBuffer(source_track->handles[0]));
    assert(alIsSource(handle));

    alSourcePlay(handle);
}

void g::snd::source::seek(float time)
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