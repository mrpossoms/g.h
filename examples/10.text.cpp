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

    g::gfx::mesh<vertex::pos_uv_norm> plane;
    g::gfx::font ubuntu;

    virtual bool initialize()
    {
        ubuntu = g::gfx::font_factory{}.from_true_type("data/fonts/UbuntuMono-B.ttf", 32);
        plane = g::gfx::mesh_factory{}.plane();

        return true;
    }

    virtual void update(float dt)
    {
        static char c = 'e';
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto I = mat4::I();

        auto zero = ubuntu.char_map[c];

        plane.using_shader(assets.shader("basic_gui.vs+basic_font.fs"))
        ["u_model"].mat4(I)
        ["u_view"].mat4(I)
        ["u_proj"].mat4(I)
        ["u_font_color"].vec4({1, 1, 1, 1})
        ["u_uv_top_left"].vec2(zero.uv_top_left)
        ["u_uv_bottom_right"].vec2(zero.uv_bottom_right)
        ["u_texture"].texture(ubuntu.face)
        .draw_tri_fan();

        // c++;
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
    core.start({ "10.text", true, 512, 512 });
// #endif

    return 0;
}
