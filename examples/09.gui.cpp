#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;


struct my_core : public g::core
{
    g::asset::store assets;
    float t = 0;

    virtual bool initialize()
    {
        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g::ui::layer root(&assets, "basic_gui.vs+basic_gui.fs");

        root.shader().draw_tri_fan();

        auto button = root.child({0.5f + (cosf(t) * 0.05f), 0.5f + (cosf(t) * 0.05f)}, {0, 0, -0.1f});

        if (button.intersects({0, 0, -1}, {0, 0, 1}))
        {
            t += dt;
        }

        button.shader().draw_tri_fan();
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
    core.start({ "09.gui", true, 512, 512 });
// #endif

    return 0;
}
