#include <g.h>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

struct my_core : public g::core
{
    g::snd::track tone_track;
    g::snd::track generator_track;
    g::snd::source* tone_source;
    g::snd::source* generator_source;
    float t = 0;
    float modulation = 10;

    virtual bool initialize()
    {
        g::snd::track::description desc;

        int16_t pcm[desc.frequency];
        for (unsigned ti = 0; ti < desc.frequency; ti++)
        {
            auto p = ti / (float)desc.frequency;
            auto fall_off = std::min<float>(std::min<float>(p * 100.f, 1.f), (1-p) * 100.f);
            pcm[ti] = fall_off * 20000 * sin(modulation * p);
        }

        tone_track = g::snd::track_factory::from_pcm_buffer(pcm, sizeof(pcm), desc);
        tone_source = new g::snd::source(&tone_track);

        auto modulated = [&](const g::snd::track::description& desc, float t_0, float t_1)
        {
            std::vector<uint8_t> pcm;
            auto dt = 1 / (float)desc.frequency;
            for (float t = t_0; t <= t_1; t+=dt)
            {
                auto signal = sin(1600 * t) * cos(modulation * t);
                int16_t sample = 20000 * signal;
                pcm.push_back(sample & 0x00FF); // lo
                pcm.push_back(sample >> 8); // hi
            }

            return pcm;
        };

        generator_track = g::snd::track_factory::from_generator(modulated, {});
        generator_source = new g::snd::source(&generator_track);

        generator_source->play();

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            tone_source->play();
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS)
        {
            modulation += dt * 100;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS)
        {
            modulation -= dt * 100;
        }

        modulation = std::max(0.f, modulation);

        generator_source->update(t, dt);

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
    core.start({ "0425.sound", true, 512, 512 });
// #endif

    return 0;
}
