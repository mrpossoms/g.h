#version 410
in vec4 v_screen_pos;
in vec2 v_uv;

uniform sampler2D u_texture;

out vec4 color;

void main (void)
{
    color = texture(u_texture, v_uv);
}
