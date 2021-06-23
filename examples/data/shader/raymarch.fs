#version 410

in vec4 v_screen_pos;
in vec2 v_uv;

out vec4 color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main(void)
{
	vec4 vs_ray = inverse(u_proj) * vec4(v_screen_pos.xy, 100000.0, 1.0);

	color = vec4(vs_ray.xyz, 1.0);
}