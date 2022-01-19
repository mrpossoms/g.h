#include <g.h>

#include <algorithm>

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

        std::vector<int8_t> v[3];
        for (unsigned i = 1024; i--;)
        {
            v[0].push_back(rand() % 255 - 128);
            v[1].push_back(rand() % 255 - 128);
            v[2].push_back(rand() % 255 - 128);
        }

        auto sdf = [&](const vec<3> p) -> float {
            //auto d = p.dot(p) - 0.75f;
            //d += g::gfx::perlin(p * 9, v) * 0.01;
            // d += g::gfx::perlin(p * 11, v) * 0.01;
            // d += g::gfx::perlin(p * 3, v) * 0.1;


            auto d = p[1] - 2;
            d += g::gfx::perlin(p*4.03, v[0]) * 0.125;
            d += g::gfx::perlin(p*1.96, v[1]) * 0.25;
            d += g::gfx::perlin(p*0.1, v[2]) * 9;

            // d = std::max<float>(d, -1);

            return d;
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


        vec<3> corners[2] = { {-10, -2, -10}, {10, 10, 10} };

        time_t start = time(NULL);
        terrain.from_sdf(sdf, generator, corners, 61);
        time_t end = time(NULL);

        std::cerr << "processing time: " << end - start << std::endl;

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
