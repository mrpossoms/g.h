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
        ""
        "vec3 screen_to_world(mat4 proj, mat4 view, vec3 p)"
        "{"
        "   float a = proj[3][2];"
        "   float b = proj[2][2];"
        "   float z = -1.0 / ((p[2] * a) - b);"
        "   float w = z * a;"
        "   vec4 p_w = vec4(p.xy * w, z, 1);"
        "   return (inverse(view) * p_w).xyz;"
        "}"
        ""
        "vec3 world_to_screen(mat4 proj, mat4 view, vec3 p)"
        "{"
        "   return (proj * view * vec4(p, 1.0)).xyz;"
        "}"
        ""
        "vec3 to_screen(sampler2D tex)"
        "{"
        "   return vec3(v_uv * 2 - vec2(1.0), texture(tex, v_uv).r);"
        "}"
        ""
        "void main (void)"
        "{"
        "   float bias = 0.00006;"
        "   vec3 cam_w = screen_to_world(u_proj, u_view, to_screen(u_camera_depth));"
        "   vec3 light_s = world_to_screen(u_light_proj, u_light_view, cam_w);"
        "   float light_depth = texture(u_camera_depth, (light_s.xy * 2.0 - 1.0)).r;"
        //"   vec3 cam_w = screen_to_world(u_proj, u_view, to_screen(u_cam_depth));"
        "   if (light_depth < light_s.z)"
        "   {"
        "       color = vec4(vec3(0.0), 1.0);"
        "   }"
        "   else"
        "   {"
        "       color = vec4(1.0);"
        "   }"
        // "color = vec4(to_screen(u_camera_depth), 1.0);"
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
