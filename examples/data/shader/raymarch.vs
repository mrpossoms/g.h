#version 410
in vec3 a_position;
in vec2 a_uv;
in vec3 a_normal;

out vec4 v_screen_pos;
out vec2 v_uv;

out mat3 v_view_rotation;
out vec3 v_view_pos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main(void)
{
	v_view_rotation = mat3(
		normalize(u_view[0].xyz),
		normalize(u_view[1].xyz),
		normalize(u_view[2].xyz),
	);

	v_view_pos = -u_view[3].xyz;

	v_uv = a_uv;
	gl_Position = v_screen_pos = vec4(a_position, 1.0);
}
