#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

struct volumetric : public g::core
{
    const std::string vs_src =
    "#version 410\n"
    "in vec3 a_position;"
    "uniform mat4 u_model;"
    "uniform mat4 u_view;"
    "uniform mat4 u_proj;"
    "out vec3 v_position;"
    "void main (void) {"
    "v_position = a_position * 0.5 + vec3(0.5);"
    "gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
    "}";

    const std::string fs_src =
    "#version 410\n"
    "in vec3 v_position;"
    "uniform sampler3D u_voxels;"
    "out vec4 color;"
    "void main (void) {"
    // "color = texture(u_voxels, v_position);"
    "color.rgb = v_position;"
    "color.a = color.r;"
    "}";

    g::gfx::mesh<g::gfx::vertex::pos> slices;
    g::game::camera_perspective cam;
    g::gfx::shader basic_shader;
    float t, sub_step = 0.1f;

    GLuint voxels;

    virtual bool initialize()
    {
        slices = g::gfx::mesh_factory::slice_cube(2);

        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        cam.position = { 0, 1, 0 };

        glGenTextures(1, &voxels);
        glBindTexture(GL_TEXTURE_3D, voxels);

        const unsigned w=3, h=3, d=3;
        float data[w][h][d];

        for (unsigned i = 0; i < w; i++)
        for (unsigned j = 0; j < h; j++)
        for (unsigned k = 0; k < d; k++)
        {
            // if (i == w / 2 && j == h / 2 && k == d / 2) { data[i][j][k] = 1.f; }
            if ((i + j + k) % 2 == 0) { data[i][j][k] = 1.f; }
            else { data[i][j][k] = 0.f; }
        }

        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            GL_RED,
            w, h, d,
            0,
            GL_RED,
            GL_FLOAT,
            data
        );

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        return true;
    }

    virtual void update(float dt)
    {

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * -dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.position += cam.up() * dt;


        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);

        xmath::mat<4, 1> v = {
            {0}, {0}, {1000}, {1}
        };

        // auto p = cam.projection() * v;
        // p /= p[0][3];

        // auto u = cam.projection().invert() * p;
        // u /= u[0][3];


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxels);
        slices.using_shader(basic_shader)
             .set_camera(cam)
             ["u_model"].mat4(mat4::translation({0, 0, 2}))
             ["u_voxels"].int1(0)
             .draw<GL_TRIANGLE_FAN>();

        t += dt;
    }
};


int main (int argc, const char* argv[])
{
    volumetric game;

    game.start({ "volume", true, 1024, 768 });

    return 0;
}
