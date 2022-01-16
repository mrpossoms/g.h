#ifdef GL_ES
precision mediump float;
#endif

in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

uniform mat4 u_light_view;
uniform mat4 u_light_proj;

out vec4 v_color;
out vec3 v_normal;
out vec4 v_light_proj_pos;

void main (void)
{
 vec4 v_world_pos = u_model * vec4(a_position, 1.0);
	gl_Position = u_proj * u_view * v_world_pos;

	v_color = a_color * vec4(1.0/254.0);

    mat3 model_rot = mat3(normalize(u_model[0].xyz), normalize(u_model[1].xyz), normalize(u_model[2].xyz));
    v_normal = normalize(mat3(u_light_view) * model_rot * a_normal);
	v_light_proj_pos = u_light_proj * u_light_view * v_world_pos;
	v_light_proj_pos /= v_light_proj_pos.w;
	v_light_proj_pos = (v_light_proj_pos + 1.0) / 2.0;
}
