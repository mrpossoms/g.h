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


void effect::shadow(const framebuffer& shadow_map, const framebuffer& camera_fb, game::camera& light, game::camera& cam)
{
    static shader shadow_shader;
    static std::string shadow_map_fs_src =
    "in vec2 v_uv;"
    "in vec4 v_light_proj_pos;"
    "uniform mat4 u_camera_unproject;"
    "uniform sampler2D u_shadow_map;"
    "uniform sampler2D u_camera_depth;"
    "out vec4 color;"
    "void main (void)"
    "{"
    "   float bias = 0.00006;"
    "   float depth = texture(u_camera_depth, v_uv).r;"
    "   float shadowing = 0.0;"
    "   for(float y = -2.0; y <= 2.0; y++)"
    "   for(float x = -2.0; x <= 2.0; x++)"
    "   {"
    "       float sampled_depth = texture(u_shadow_map, v_uv + (vec2(x, y) * 0.0005)).r;"
    "       if (depth <= sampled_depth)"
    "       {"
    "           shadowing += 1.0 / 25.0;"
    "       }"
    "   }"
    "   color = vec4(vec3(depth), 1.0);"
    "}";

    if (!shadow_shader.is_initialized())
    {
        shadow_shader = shader_factory{}
            .add_src<GL_VERTEX_SHADER>(post_proc_vs_src)
            .add_src<GL_FRAGMENT_SHADER>(shadow_map_fs_src)
            .create();
    }


    fullscreen_quad().using_shader(shadow_shader)
    ["u_shadow_map"].texture(shadow_map.depth)
    ["u_camera_depth"].texture(camera_fb.depth)
    .draw<GL_TRIANGLE_FAN>();
}
