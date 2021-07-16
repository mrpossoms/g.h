#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

const unsigned w=64, h=64, d=64;
char* data;

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
    "out mat4 v_inv_rotation;"
    "void main (void) {"
    "v_uvw = (u_rotation * vec4(a_position, 1.0)).xyz + vec3(0.5);"
    "v_inv_rotation = inverse(u_rotation);"
    "gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);"
    "}";

    const std::string fs_src =
    "#version 410\n"
    "in vec3 v_uvw;"
    "in mat4 v_inv_rotation;"
    "uniform sampler3D u_voxels;"
    "uniform vec3 u_uvw_step;"
    "out vec4 color;"
    "void main (void) {"
    "float v = texture(u_voxels, v_uvw).r;"
    "vec3 dc = u_uvw_step;"
    "float n_x = texture(u_voxels, v_uvw - vec3(dc.x, 0.0, 0.0)).r - texture(u_voxels, v_uvw + vec3(dc.x, 0.0, 0.0)).r;"
    "float n_y = texture(u_voxels, v_uvw - vec3(0.0, dc.y, 0.0)).r - texture(u_voxels, v_uvw + vec3(0.0, dc.y, 0.0)).r;"
    "float n_z = texture(u_voxels, v_uvw - vec3(0.0, 0.0, dc.z)).r - texture(u_voxels, v_uvw + vec3(0.0, 0.0, dc.z)).r;"
    "vec3 normal = (v_inv_rotation * vec4(n_x, n_y, n_z, 1.0)).xyz;"
    "float shade = dot(normalize(vec3(1.0, 1.0, 0.0)), normal);"
    "color = vec4((normal * 0.5 + vec3(0.5)) * shade, v);"
    "}";


    g::game::camera_perspective cam;
    g::gfx::shader basic_shader;
    g::gfx::shader viz_shader;
    g::gfx::texture vox;
    g::gfx::primative::volume_slicer slicer;
    float t = 0;

    GLuint voxels;

    virtual bool initialize()
    {
        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        slicer = g::gfx::primative::volume_slicer(1000); 

        cam.position = { 0, 1, 10 };

        data = new char[w * h * d * 1];

        auto bytes_per_plane = 1 * h * d;


        auto sq_rad = powf(w>>1, 2.f);
        int hw = w >> 1, hh = h >> 1, hd = d >> 1;
        auto fill_func = [&](int i, int j, int k, char* data) {
            auto dw = i - hw, dh = j - hh, dd = k - hd;
            auto sq_dist = (dw * dw) + (dh * dh) + (dd * dd);
            data[0] = (sq_dist < sq_rad) * 0xff;
        };

        vox = g::gfx::texture_factory{ w, h, d }.pixelated()
                                                .clamped_to_border()
                                                .components(1)
                                                .type(GL_UNSIGNED_BYTE)
                                                .fill(fill_func).create();

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
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.position += cam.up() * -dt;

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glDisable(GL_CULL_FACE);

        slicer.draw(basic_shader, vox, cam, mat4::rotation({0, 1, 0}, t));


        t += dt;
    }
};


int main (int argc, const char* argv[])
{
    volumetric game;

    game.start({ "volume", true, 1024, 768 });

    return 0;
}
