#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

const unsigned w=32, h=32, d=32;
uint8_t* data;

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
    "float v = texture(u_voxels, v_position).r;"
    "if (v <= 0.0) { discard; }"
    "const float dc = 0.001f;"
    "float n_x = texture(u_voxels, v_position - vec3(dc, 0.0, 0.0)).r - texture(u_voxels, v_position + vec3(dc, 0.0, 0.0)).r;"
    "float n_y = texture(u_voxels, v_position - vec3(0.0, dc, 0.0)).r - texture(u_voxels, v_position + vec3(0.0, dc, 0.0)).r;"
    "float n_z = texture(u_voxels, v_position - vec3(0.0, 0.0, dc)).r - texture(u_voxels, v_position + vec3(0.0, 0.0, dc)).r;"
    "vec3 normal = vec3(n_x, n_y, n_z);"
    "color = vec4(normal * 0.5 + vec3(0.5), 1.0);"
    // "color = texture(u_voxels, v_position);"
    "}";

    g::gfx::mesh<g::gfx::vertex::pos> slices;
    g::game::camera_perspective cam;
    g::gfx::shader basic_shader;
    float t, sub_step = 0.1f;

    GLuint voxels;

    virtual bool initialize()
    {
        slices = g::gfx::mesh_factory::slice_cube(1000);

        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        cam.position = { 0, 1, 1 };

        glGenTextures(1, &voxels);
        glBindTexture(GL_TEXTURE_3D, voxels);

        data = new uint8_t[w * h * d * 3];

        auto bytes_per_plane = 3 * h * d;

        int lut[999];

        for (int i = 0; i < 999; i++)
        {
            lut[i] = rand();
        }

        for (unsigned i = 0; i < w; i++)
        for (unsigned j = 0; j < h; j++)
        for (unsigned k = 0; k < d; k++)
        {
            unsigned vi = i * bytes_per_plane + (j * d * 3) + (k * 3);
            // if (i == w / 2 && j == h / 2 && k == d / 2) { data[i][j][k] = 1.f; }
            if ((i + j + k) % 2 == 0) { 
                data[vi + 0] = lut[vi % 999] & 0xff;
                data[vi + 1] = (lut[vi % 999] >> 8) & 0xff;
                data[vi + 2] = (lut[vi % 999] >> 16) & 0xff;
            }
            else
            { 
                data[vi + 0] = 0;
                data[vi + 1] = 0;
                data[vi + 2] = 0;
            }
        }

        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            GL_RGB,
            w, h, d,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            data
        );

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
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
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(-dt);
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
        xmath::vec<3> vo = {0, 0, 0};

        auto O = (cam.position - vo).unit();
        auto u = xmath::vec<3>{0, 1, 0};//cam.up();
        auto l = xmath::vec<3>::cross(O, u);

        // xmath::mat<4, 4> R = {
        //     { l[0], u[0], O[0], 0 },
        //     { l[1], u[1], O[1], 0 },
        //     { l[2], u[2], O[2], 0 },
        //     { 0,    0,    0,    1 },
        // };

        auto R = xmath::mat<4, 4>::look_at(vo, O, u).invert();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxels);

        slices.using_shader(basic_shader)
             .set_camera(cam)
             ["u_model"].mat4(mat4::translation(vo))
             ["u_voxels"].int1(0)
             .draw<GL_TRIANGLES>();

        slices.using_shader(basic_shader)
             .set_camera(cam)
             ["u_model"].mat4(R)//mat4::translation(vo))
             ["u_voxels"].int1(0)
             .draw<GL_TRIANGLES>();

        t += dt;
    }
};


int main (int argc, const char* argv[])
{
    volumetric game;

    game.start({ "volume", true, 1024, 768 });

    return 0;
}
