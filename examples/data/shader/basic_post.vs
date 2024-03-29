in vec3 a_position;
in vec2 a_uv;
in vec3 a_normal;

out vec4 v_screen_pos;
out vec2 v_uv;

void main(void)
{
	v_uv = a_uv;
	gl_Position = v_screen_pos = vec4(a_position, 1.0);
}
