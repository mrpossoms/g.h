#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

const unsigned w=8, h=8, d=8;
uint8_t* data;

struct volumetric : public g::core
{
    const std::string vs_src =
    "#version 410\n"
    "in vec3 a_position;"
    "uniform mat4 u_model;"
    "uniform mat4 u_view;"
    "uniform mat4 u_proj;"
    "uniform mat4 u_rotation;"
    "out vec3 v_uvw;"
    "void main (void) {"
    "v_uvw = (u_rotation * vec4(a_position, 1.0)).xyz + vec3(0.5);"
    // "v_uvw = (inverse(u_rotation) * vec4(a_position, 1.0)).xyz + vec3(0.5);"
    "gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);"
    "}";

    const std::string fs_src =
    "#version 410\n"
    "in vec3 v_uvw;"
    "uniform sampler3D u_voxels;"
    "uniform float u_tex_coord_step;"
    "out vec4 color;"
    "void main (void) {"
    "float v = texture(u_voxels, v_uvw).r;"
    // "if (v <= 0.0) { discard; }"
    "float dc = u_tex_coord_step;"
    "float n_x = texture(u_voxels, v_uvw - vec3(dc, 0.0, 0.0)).r - texture(u_voxels, v_uvw + vec3(dc, 0.0, 0.0)).r;"
    "float n_y = texture(u_voxels, v_uvw - vec3(0.0, dc, 0.0)).r - texture(u_voxels, v_uvw + vec3(0.0, dc, 0.0)).r;"
    "float n_z = texture(u_voxels, v_uvw - vec3(0.0, 0.0, dc)).r - texture(u_voxels, v_uvw + vec3(0.0, 0.0, dc)).r;"
    "vec3 normal = vec3(n_x, n_y, n_z);"
    // "color = vec4(normal * 0.5 + vec3(0.5), v);"
    // "color = texture(u_voxels, v_uvw) * vec4;"
    "color = vec4(v_uvw, v);"
    "}";

    const std::string fs_vis_src =
    "#version 410\n"
    "in vec3 v_uvw;"
    "uniform sampler3D u_voxels;"
    "out vec4 color;"
    "void main (void) {"
    "color = vec4(0.0, 1.0, 1.0 - v_uvw.z, 1.0/256.0);"
    "}";

    g::gfx::mesh<g::gfx::vertex::pos> slices;
    g::gfx::mesh<g::gfx::vertex::pos> low_res;
    g::game::camera_perspective cam;
    g::gfx::shader basic_shader;
    g::gfx::shader viz_shader;
    float t, sub_step = 0.1f;

    GLuint voxels;

    virtual bool initialize()
    {
        low_res = g::gfx::mesh_factory::slice_cube(100);
        slices = g::gfx::mesh_factory::slice_cube(1000);

        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        viz_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_vis_src)
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

        auto sq_rad = powf(w>>1, 2.f);
        int hw = w >> 1, hh = h >> 1, hd = d >> 1;
        for (int i = 0; i < w; i++)
        for (int j = 0; j < h; j++)
        for (int k = 0; k < d; k++)
        {
            unsigned vi = i * bytes_per_plane + (j * d * 3) + (k * 3);

            auto sq_dist = powf(i - hw, 2.f) + pow(j - hh, 2.f) + pow(k - hd, 2.f);
            if (sq_dist < sq_rad)
            {
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
        xmath::vec<3> vo = {cos(t), 0, sin(t)};

        // auto O = (cam.position - vo).unit();
        auto O = cam.forward();
        auto u = xmath::vec<3>{0, 1, 0};//cam.up();
        auto l = xmath::vec<3>::cross(O, u);
        u = xmath::vec<3>::cross(O, l);

        // xmath::mat<4, 4> R = {
        //     { l[0], u[0], O[0], 0 },
        //     { l[1], u[1], O[1], 0 },
        //     { l[2], u[2], O[2], 0 },
        //     { 0,    0,    0,    1 },
        // };

        auto R = xmath::mat<4, 4>::look_at(vo, O, u).invert();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxels);

        // slices.using_shader(basic_shader)
        //      .set_camera(cam)
        //      ["u_model"].mat4(mat4::translation(vo))
        //      ["u_rotation"].mat4(mat4::I())
        //      ["u_voxels"].int1(0)
        //      .draw<GL_TRIANGLES>();


        glDisable(GL_DEPTH_TEST);
        low_res.using_shader(viz_shader)
             .set_camera(cam)
             ["u_model"].mat4(mat4::translation(vo) * R)//mat4::translation(vo))
             ["u_rotation"].mat4(R)
             ["u_voxels"].int1(0)
             .draw<GL_TRIANGLES>();

        glEnable(GL_DEPTH_TEST);
        slices.using_shader(basic_shader)
             .set_camera(cam)
             ["u_model"].mat4(mat4::translation(vo) * R)//mat4::translation(vo))
             ["u_rotation"].mat4(R)
             ["u_tex_coord_step"].flt(1.f/static_cast<float>(w >> 2))
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
