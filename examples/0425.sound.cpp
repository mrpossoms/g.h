#include <g.h>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

struct my_core : public g::core
{
    g::asset::store assets;

    g::snd::track tone_track;
    g::snd::track gun_track;
    g::snd::track streaming_track;
    g::snd::source* streaming_source;
    g::snd::source* tone;
    g::snd::source_ring* gun;
    float t = 0;
    float modulation = 10;
    float trailing_modulation = 10;
    bool trigger_pulled = false;

    virtual bool initialize()
    {
        g::snd::track::description desc;

        int16_t* pcm = new int16_t[desc.frequency];
        for (unsigned ti = 0; ti < desc.frequency; ti++)
        {
            auto p = ti / (float)desc.frequency;
            auto fall_off = std::min<float>(std::min<float>(p * 100.f, 1.f), (1-p) * 100.f);
            pcm[ti] = fall_off * 30000 * (sin(1000 * p) + sin(p * p * 2000)) * 0.5;
        }

        tone_track = g::snd::track_factory::from_pcm_buffer(pcm, sizeof(pcm), desc);
        // gun_track = g::snd::track_factory::from_wav("data/snd/gun.aiff");

        auto modulated = [&](const g::snd::track::description& desc, float t_0, float t_1)
        {
            std::vector<uint8_t> pcm;
            auto dt = 1 / (float)desc.frequency;
            auto duration = t_1 - t_0;
            for (float t = t_0; t <= t_1; t+=dt)
            {
                auto p = (t - t_0) / duration;
                auto mod = trailing_modulation * (1-p) + modulation * p;
                auto signal = sin(1600 * t) * cos(mod * t);
                int16_t sample = 20000 * signal;
                pcm.push_back(sample & 0x00FF); // lo
                pcm.push_back(sample >> 8); // hi


            }
            trailing_modulation = modulation;

            return pcm;
        };

        streaming_track = g::snd::track_factory::from_ogg("data/snd/norty.ogg");
        // streaming_track = g::snd::track_factory::from_generator(modulated, {});
        streaming_source = new g::snd::source(&streaming_track);

        tone = new g::snd::source(&tone_track);
        gun = new g::snd::source_ring(&assets.sound("gun.aiff"), 3);
        streaming_source->play();

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_G) == GLFW_PRESS)
        {
            if (!trigger_pulled) { gun->play(); }
            trigger_pulled = true;
        }
        else
        {
            trigger_pulled = false;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_T) == GLFW_PRESS)
        {
            tone->play();
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            streaming_source->seek(streaming_source->last_t + 1);
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            streaming_source->seek(streaming_source->last_t - 1);
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS)
        {
            modulation += dt * 10;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS)
        {
            modulation -= dt * 10;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            streaming_source->stop();
        }

        modulation = std::max<float>(0.f, modulation);

        streaming_source->update();
        gun->update();
        tone->update();

        t += dt;
    }
};

my_core core;

// void main_loop() { core.tick(); }

int main (int argc, const char* argv[])
{

// #ifdef __EMSCRIPTEN__
//  core.running = false;
//  core.start({ "04.basic_draw", true, 512, 512 });
//  emscripten_set_main_loop(main_loop, 144, 1);
// #else
    core.start({ 
        .name = argv[0],
        .gfx = {
            .display = true,
            .width = 512,
            .height = 512 
        }
    });
// #endif

    return 0;
}
