in vec4 v_screen_pos;
in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec4 u_font_color;
uniform vec2 u_uv_top_left;
uniform vec2 u_uv_bottom_right;

out vec4 color;

void main (void)
{
    vec2 uv = v_uv;

    float a = texture(u_texture, uv).r;
    color = u_font_color;
    color.a *= a;
}
