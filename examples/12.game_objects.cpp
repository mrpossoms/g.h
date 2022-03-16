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
    g::game::camera_perspective cam;

    std::function<vertex::pos_uv_norm(const texture& t, int x, int y)> vertex_generator;

    g::game::object plane0;

    virtual bool initialize()
    {
        plane0 = g::game::object{&assets, "data/plane0.yaml", {
            {"traits", {
                { "thrust", 100},
                { "roll_power", 1},
                { "yaw_power", 1},
                { "pitch_power", 1}}},
            {"textures", {
                { "color", "plane0/color.png"}}},
            {"geometry", {
                { "mesh", "plane0/mesh.obj"}}}
        }};

        cam.position = {0, 30, 0};
        //glDisable(GL_CULL_FACE);

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

        auto model = mat4::rotation({0, 1, 0}, t + M_PI) * mat4::translation({0, -1, -2});

        plane0.geometry("mesh").using_shader(assets.shader("basic_gui.vs+basic_texture.fs"));
    }

    float t;
};

my_core core;

int main (int argc, const char* argv[])
{
    core.start({ "12.game_objects", true, 512, 512 });

    return 0;
}
