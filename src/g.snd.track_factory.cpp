#include "g.snd.h"

static ALCdevice* dev;
static ALCcontext* ctx;

constexpr static ALenum formats[2][g::snd::bit_depth::count] = {
    {AL_FORMAT_MONO8, AL_FORMAT_MONO16},
    {AL_FORMAT_STEREO8, AL_FORMAT_STEREO16},
};


static void init_openal()
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


namespace g
{
namespace snd
{

track track_factory::from_pcm_buffer(void* buf, size_t size, const track::description& desc)
{
    init_openal();

    ALuint al_buf;

    {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << G_TERM_RED << "pre check error: " << std::string(alGetString(error)) << std::endl;
            exit(1);
        }
    }

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

    alBufferData(al_buf, formats[desc.channels - 1][desc.depth - 1], buf, size, desc.frequency);

    return { desc, std::vector<ALuint>{al_buf} };
}


track track_factory::from_generator(track::pcm_generator generator, const track::description& desc)
{
    init_openal();

    std::vector<ALuint> bufs;

    {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << G_TERM_RED << "pre check error: " << std::string(alGetString(error)) << std::endl;
            exit(1);
        }
    }

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


track track_factory::from_ogg(const std::string& path)
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


track track_factory::from_wav(const std::string& path)
{
    AudioFile<int16_t> wav(path);

    track::description desc;

    desc.frequency = wav.getSampleRate();
    desc.looping   = false;
    desc.channels  = wav.getNumChannels();
    desc.depth     = (bit_depth)(wav.getBitDepth() / 8);

    if (std::string::npos != path.find(".looping"))
    {
        desc.looping = true;
    }

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

} // namespace snd

} // namespace g