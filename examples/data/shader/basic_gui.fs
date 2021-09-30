#version 410
in vec4 v_screen_pos;
in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec4 u_color;
uniform vec4 u_border_color;
uniform float u_border_thickness;
out vec4 color;

void main (void)
{
    color = u_color;

    color += texture(u_texture, vec2(1.0) - v_uv);
    
    float bt = u_border_thickness;
    color += u_border_color * float(v_uv.x < bt || v_uv.x > (1.0 - bt) || v_uv.y < bt || v_uv.y > (1.0 - bt));
}
