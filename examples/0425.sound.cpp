#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

struct my_core : public g::core
{
    g::snd::track tone;
    g::snd::source* tone_source;

    virtual bool initialize()
    {
        g::snd::track::desc desc;

        int16_t pcm[desc.frequency];
        for (unsigned ti = 0; ti < desc.frequency; ti++)
        {
            auto p = ti / (float)desc.frequency;
            auto fall_off = std::min<float>(std::min<float>(p * 100.f, 1.f), (1-p) * 100.f);
            pcm[ti] = fall_off * 20000 * sin(800 * p);
        }

        tone = g::snd::track_factory::from_pcm_buffer(pcm, sizeof(pcm), desc);
        tone_source = new g::snd::source(tone);

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
