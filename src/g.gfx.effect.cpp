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

void effect::shadow(const framebuffer& shadow_map, game::camera& light, game::camera& cam)
{

    // TODO: the assumption that the appropriate textel in depth_map can be
    // sampled to loop up a uv is not correct. I think the correct approach is
    // to involve the vertex shader like it has been done traditionally
    static shader shadow_shader;

    static std::string shadow_map_vs_src =
    "in vec3 a_position;"
    "uniform mat4 u_model;"
    "uniform mat4 u_view;"
    "uniform mat4 u_proj;"
    "uniform mat4 u_light_view;"
    "uniform mat4 u_light_proj;"
    "out vec4 v_light_proj_pos;"
    "void main (void)"
    "{"
    "   vec4 v_world_pos = u_model * vec4(a_position, 1.0);"
    "   gl_Position = u_proj * u_view * v_world_pos;"
    "   v_light_proj_pos = u_light_proj * u_light_view * v_world_pos;"
    "   v_light_proj_pos /= v_light_proj_pos.w;"
    "   v_light_proj_pos = (v_light_proj_pos + 1.0) / 2.0;"
    "}";

    static std::string shadow_map_fs_src =
    "in vec4 v_light_proj_pos;"
    "uniform sampler2D u_shadow_map;"
    "out vec4 color;"
    "void main (void)"
    "{"
    "   float bias = 0.00006;"
    "   float depth = v_light_proj_pos.z - bias;"
    "   float shadowing = 0.0;"
    "   for(float y = -2.0; y <= 2.0; y++)"
    "   for(float x = -2.0; x <= 2.0; x++)"
    "   {"
    "       float sampled_depth = texture(u_shadow_map, v_light_proj_pos.xy + (vec2(x, y) * 0.0005)).r;"
    "       if (depth <= sampled_depth)"
    "       {"
    "           shadowing += 1.0 / 25.0;"
    "       }"
    "   }"
    "   color = vec4(vec3(shadowing), 1.0);"
    "}";



    if (!shadow_shader.is_initialized())
    {
        shadow_shader = shader_factory{}
            .add_src<GL_VERTEX_SHADER>(shadow_map_vs_src)
            .add_src<GL_FRAGMENT_SHADER>(shadow_map_fs_src)
            .create();
    }

    // TODO: set appropriate blending

    auto model = mat<4, 4>::I();

    fullscreen_quad().using_shader(shadow_shader)
    .set_camera(cam)
    ["u_shadow_map"].texture(shadow_map.depth)
    ["u_model"].mat4(model)
    ["u_light_view"].mat4(light.view())
    ["u_light_proj"].mat4(light.projection())
    .draw<GL_TRIANGLE_FAN>();
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
