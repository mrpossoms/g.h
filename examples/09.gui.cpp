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
    g::game::camera_perspective cam;

    virtual bool initialize()
    {
        cam.position = { 0, 0, -2 };
        cam.orientation = xmath::quat<>::from_axis_angle({ 0, 1, 0 }, M_PI);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glPointSize(10);
        return true;
    }

    virtual void update(float dt)
    {
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
        cam.aspect_ratio = g::gfx::aspect();

        auto pointer = g::ui::pointer_from_mouse(&cam);
        g::ui::layer root(&assets, "basic_gui.vs+basic_gui.fs");

        root.set_font("UbuntuMono-B.ttf");
        root.set_pointer(&pointer);
        root.using_shader().set_camera(cam)
        ["u_border_thickness"].flt(0.01)
        ["u_border_color"].vec4({1, 1, 1, 1})
        ["u_color"].vec4({0.1, 0.1, 0.1, 1})
        .draw_tri_fan();

        // auto ray_o = g::ui::origin_from_mouse(&cam);
        auto button0 = root.child(vec<2>{ 0.25f , 0.125f} + vec<2>{ 0.25f , 0.125f} * (cosf(t[0] * 2) * 0.5f), { -0.55f, 0, -0.1f });

        for (auto event : button0)
        {
            switch(event.event)
            {
                case g::ui::event::hover:
                    t[0] += dt;
                    break;
                case g::ui::event::draw:
                {        
                    event.draw.set_camera(cam).draw_tri_fan();
                    auto b0_text = button0.child({ 0.8, 0.8, 1 }, { 0, 0, -0.01f }).set_shaders("basic_gui.vs+basic_font.fs");
                    b0_text.text("hover", cam).draw<GL_TRIANGLES>();
                }
                    break;
            }
        }
        
        auto button1 = root.child({ 0.25f + (cosf(t[1] * 2) * 0.05f), 0.25f + (cosf(t[1] * 2) * 0.05f) }, { 0.55f, 0, -0.1f });
        button1.using_shader().set_camera(cam).draw_tri_fan();
        if (button1.select(pointer, 1))
        {
            t[1] += dt;
        }
        auto b1_text = button1.child({0.8, 0.8, 1}, {0, 0, -0.01f}).set_shaders("basic_gui.vs+basic_font.fs");
        b1_text.set_font("OpenSans-Regular.ttf").text("click & hold!", cam).draw<GL_TRIANGLES>();

        // debug::print(&cam).color({ 0, 1, 0, 1 }).ray({ 0, 0, 0 }, { 0, 0, 1 });

        //debug::print(&cam).color({ 0, 1, 0, 1 }).point(pointer.position + (pointer.direction * vec<3>{1, 1, 1}));
        debug::print(&cam).color({ 1, 0, 0, 1 }).ray(pointer.position + pointer.direction, pointer.direction);
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
