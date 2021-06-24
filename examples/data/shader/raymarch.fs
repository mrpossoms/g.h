#version 410

in vec4 v_screen_pos;
in vec2 v_uv;

in mat3 v_view_rotation;
in vec3 v_view_pos;

out vec4 color;

uniform mat4 u_view;

void main(void)
{
	vec3 d = v_view_rotation * normalize(vec3(v_screen_pos.xy, 1.0));
	vec3 o = v_view_pos;

	for (int t = 0; t < 20; t++)
	{

	}
}
