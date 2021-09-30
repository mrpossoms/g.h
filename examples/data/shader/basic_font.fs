#version 410
in vec4 v_screen_pos;
in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec4 u_font_color;
uniform vec2 u_uv_top_left;
uniform vec2 u_uv_bottom_right;

out vec4 color;

void main (void)
{
    vec2 uv = (vec2(1.0) - v_uv) * (u_uv_bottom_right - u_uv_top_left) + u_uv_top_left;

    float a = texture(u_texture, uv).r;
    color = u_font_color;
    color.a *= a;//vec4(1.0, 1.0, 1.0, a);
}
