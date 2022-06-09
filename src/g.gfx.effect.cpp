#include "g.gfx.h"

static std::string post_proc_vs_src =
"in vec3 a_position;"
"in vec2 a_uv;"
"in vec3 a_normal;"
"out vec4 v_screen_pos;"
"out vec2 v_uv;"
"void main(void)"
"{"
"   v_uv = a_uv;"
"   gl_Position = v_screen_pos = vec4(a_position, 1.0);"
"}";

using namespace g::gfx;

mesh<vertex::pos_uv_norm>& effect::fullscreen_quad()
{
    static mesh<vertex::pos_uv_norm> quad;

    if (!quad.is_initialized())
    {
        quad = mesh_factory::plane();
    }

    return quad;
}


void effect::blit(const framebuffer& fb)
{
    static shader blit_shader;
    static std::string blit_fs_src =
    "in vec4 v_screen_pos;"
    "in vec2 v_uv;"
    "uniform sampler2D u_texture;"
    "out vec4 color;"
    "void main (void)"
    "{"
    "   color = texture(u_texture, vec2(v_uv.x, 1 - v_uv.y));"
    "}";

    if (!blit_shader.is_initialized())
    {
        blit_shader = shader_factory{}
            .add_src<GL_VERTEX_SHADER>(post_proc_vs_src)
            .add_src<GL_FRAGMENT_SHADER>(blit_fs_src)
            .create();
    }

    fullscreen_quad().using_shader(blit_shader)
    ["u_texture"].texture(fb.color)
    .draw<GL_TRIANGLE_FAN>();
}


void effect::shadow(const framebuffer& light_depth, const framebuffer& camera_depth, game::camera& light, game::camera& cam)
{
    static shader shadow_shader;
    static std::string light_depth_fs_src =
    "in vec2 v_uv;"
    "in vec4 v_light_proj_pos;"
    // "uniform mat4 u_camera_unproject;"
    // "uniform mat4 u_view_inv;"
    "uniform vec3 u_cam_pos;"
    "uniform mat4 u_view;"
    "uniform mat4 u_proj;"
    // "uniform mat4 u_light_view_inv;"
    "uniform vec3 u_light_pos;"
    "uniform mat4 u_light_view;"
    "uniform mat4 u_light_proj;"
    "uniform sampler2D u_light_depth;"
    "uniform sampler2D u_camera_depth;"
    "out vec4 color;"
    "vec3 unproject(vec4 p, mat4 P)"
    "{"
    "   float z = -1.0 / ((p[2] * P[3][2]) - P[2][2]);"
    "   return "
    "}"
    ""
    "void main (void)"
    "{"
    "   float bias = 0.00006;"
    "   vec3 light_pos = (u_light_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;"
    "   vec3 cam_pos = (u_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;"
    "   vec3 cam_to_light = light_pos - cam_pos;"
    // TODO: recover the view position from the camera's view to
    // query the light depth map
    "   "
    "}\n";

    if (!shadow_shader.is_initialized())
    {
        shadow_shader = shader_factory{}
            .add_src<GL_VERTEX_SHADER>(post_proc_vs_src)
            .add_src<GL_FRAGMENT_SHADER>(light_depth_fs_src)
            .create();
    }


    fullscreen_quad().using_shader(shadow_shader)
    .set_camera(cam)
    // ["u_view_inv"].mat4(cam.view().invert())
    ["u_light_pos"].vec3(light.position)
    ["u_light_view"].mat4(light.view())
    ["u_light_proj"].mat4(light.projection())
    ["u_cam_pos"].vec3(cam.position)
    ["u_light_depth"].texture(light_depth.depth)
    ["u_camera_depth"].texture(camera_depth.depth)
    .draw<GL_TRIANGLE_FAN>();
}
