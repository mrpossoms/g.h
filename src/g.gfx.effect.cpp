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
