#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

struct my_cam : public g::game::camera
{
    virtual mat<4, 4> view() const
    {
        return mat<4, 4>::I();
    }

    virtual mat<4, 4> projection() const
    {
        return mat<4, 4>::I();
    }

};

struct my_core : public g::core
{
    g::asset::store assets;
    float t[2];

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

        my_cam cam;
        auto button0 = root.child({0.25f + (cosf(t[0]) * 0.05f), 0.25f + (cosf(t[0]) * 0.05f)}, {-0.55f, 0, -0.1f});
        auto button1 = root.child({0.25f + (cosf(t[1]) * 0.05f), 0.25f + (cosf(t[1]) * 0.05f)}, {0.55f, 0, -0.1f});

        // auto ray_o = g::ui::origin_from_mouse(&cam);
        auto pointer = g::ui::pointer_from_mouse(&cam);
        if (button0.hover(pointer))
        {
            t[0] += dt;
        }

        if (button1.hover(pointer))
        {
            t[1] += dt;
        }

        button0.shader().draw_tri_fan();
        button1.shader().draw_tri_fan();
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
