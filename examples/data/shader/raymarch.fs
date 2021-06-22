#version 410

in vec4 v_screen_pos;

out vec4 v_color;

void main(void)
{

	v_color = vec4(v_screen_pos.xyz, 1.0);
}