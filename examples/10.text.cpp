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
    g::game::camera_perspective cam;

    virtual bool initialize()
    {
        // ubuntu = g::gfx::font_factory{}.from_true_type("data/fonts/OpenSans-Regular.ttf", 64);
        plane = g::gfx::mesh_factory{}.plane();
        cam.position = { 0, 0, -15 };
        cam.orientation = xmath::quat<>::from_axis_angle({ 0, 1, 0 }, M_PI);
        glDisable(GL_CULL_FACE);
        glPointSize(5);
        // glLineWidth(2);
        return true;
    }

    virtual void update(float dt)
    {
        static float t;
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto speed = 4.0f;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);

        auto I = mat4::I();

        std::string str = "Hello, world!\nthis is a test\nof newlines.";
        // auto str = "e";

        glDisable(GL_DEPTH_TEST);
        auto text = g::gfx::primative::text{assets.font("UbuntuMono-B.ttf")};

        vec<2> dims, offset;
        text.measure(str, dims, offset);
        offset = (dims * -0.5);// - offset * 0.5;

        auto model = mat4::translation({offset[0], offset[1], 0}) * mat4::rotation({0,0,1}, t+=dt) * mat4::translation({sin(t) * 10, 0, 0});

        text.draw(assets.shader("basic_gui.vs+basic_font.fs"), str, cam, model);
        // debug::print{&cam}.color({0, 1, 0, 1}).model(model).ray(vec<2>{}, dims);
        // debug::print{&cam}.color({0, 1, 0, 1}).model(model).point(offset + dims);
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
