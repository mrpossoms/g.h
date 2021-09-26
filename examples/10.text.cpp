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
    g::game::camera_perspective cam;

    virtual bool initialize()
    {
        ubuntu = g::gfx::font_factory{}.from_true_type("data/fonts/UbuntuMono-B.ttf", 64);
        plane = g::gfx::mesh_factory{}.plane();
        cam.position = { 0, 0, -10 };
        cam.orientation = xmath::quat<>::from_axis_angle({ 0, 1, 0 }, M_PI);
        cam.aspect_ratio = g::gfx::aspect();
        glDisable(GL_CULL_FACE);
        glPointSize(10);
        // glLineWidth(2);
        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto I = mat4::I();



        std::string str = "Hello, world!\nthis is a test\nof newlines. What\nDo you do when you\nhabe more lines?";
        // auto str = "e";

        glDisable(GL_DEPTH_TEST);
        auto text = g::gfx::primative::text{ubuntu};

        auto dims = text.measure(str) * 1;
        text.draw(assets.shader("basic_gui.vs+basic_font.fs"), str, cam, mat4::scale({1}) * mat4::translation({dims[0]/2, dims[1]/4, 0}));

        // auto zero = ubuntu.char_map[str[c % str.length()]];
        // plane.using_shader(assets.shader("basic_gui.vs+basic_font.fs"))
        // ["u_model"].mat4(I)
        // ["u_view"].mat4(I)
        // ["u_proj"].mat4(I)
        // ["u_font_color"].vec4({1, 1, 1, 1})
        // ["u_uv_top_left"].vec2(zero.uv_top_left)
        // ["u_uv_bottom_right"].vec2(zero.uv_bottom_right)
        // ["u_texture"].texture(ubuntu.face)
        // .draw_tri_fan();

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
