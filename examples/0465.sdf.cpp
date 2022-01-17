#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

struct my_core : public g::core
{
    g::gfx::shader basic_shader;
    g::asset::store assets;
    g::game::camera_perspective cam;
    g::gfx::mesh<g::gfx::vertex::pos_uv_norm> terrain;

    std::function<vertex::pos_uv_norm(const texture& t, int x, int y)> vertex_generator;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        terrain = g::gfx::mesh_factory{}.empty_mesh<g::gfx::vertex::pos_uv_norm>();

        auto sphere_sdf = [](const vec<3> p) -> float {
            return p.dot(p) - 1;
        };

        auto generator = [](const g::game::sdf& sdf, const vec<3>& pos) -> g::gfx::vertex::pos_uv_norm
        {
            g::gfx::vertex::pos_uv_norm v;

            v.position = pos;

            const float s = 0.1;
            vec<3> grad = {};
            vec<3> deltas[3][2] = {
                {{ s, 0, 0 }, { -s, 0, 0 }},
                {{ 0, s, 0 }, { 0, -s, 0 }},
                {{ 0, 0, s }, { 0,  0, -s }},
            };

            for (int j = 3; j--;)
            {
                vec<3> samples[2];
                samples[0] = pos + deltas[j][0];
                samples[1] = pos + deltas[j][1];
                grad[j] = sdf(samples[0]) - sdf(samples[1]);
            }

            v.normal = grad.unit();
        
            return v;
        };


        vec<3> corners[2] = { {-1, -1, -1}, {1, 1, 1} };
        terrain.from_sdf(sphere_sdf, generator, corners);

        cam.position = {0, 0, -2};
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

        terrain.using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
            ["u_model"].mat4(model)
            .set_camera(cam)
            .draw<GL_TRIANGLES>();
    }

    float t;
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
    core.start({ "046.heightmap", true, 512, 512 });
// #endif

    return 0;
}
