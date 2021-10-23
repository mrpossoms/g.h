#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

const std::string vs_src =
"#version 410\n"
"in vec3 a_position;"
"in vec2 a_uv;"
"in vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"out vec2 v_uv;"
"out vec3 v_normal;"
"void main (void) {"
"v_uv = a_uv;"
"v_normal = a_normal;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_src =
"#version 410\n"
"in vec2 v_uv;"
"in vec3 v_normal;"
"uniform sampler2D u_tex;"
"out vec4 color;"
"void main (void) {"
"color = vec4((v_normal * 0.5) + 0.5, 1.0);"
"}";


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

        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        vertex_generator = [](const texture& tex, int x, int y) -> vertex::pos_uv_norm {
            return {
                // position
                {
                    x - tex.size[0] * 0.5f,
                    static_cast<float>(tex.sample(x, y)[0] * 0.25f),
                    y - tex.size[1] * 0.5f,
                },
                // uv
                {
                    x / (float)tex.size[0],
                    y / (float)tex.size[1],
                },
                // normal
                { 0, 1, 0 }
            };
        };

        terrain = g::gfx::mesh_factory::from_heightmap(assets.tex("heightmap.png"), vertex_generator);

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

        cam.aspect_ratio = g::gfx::aspect();

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
