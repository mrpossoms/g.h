#ifdef GL_ES
precision mediump float;
#endif

in vec3 a_position;
in vec2 a_uv;
in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec2 v_uv;
out vec3 v_normal;
out vec3 v_up;
out vec3 v_left;
out vec3 v_forward;
out vec4 v_world_pos;

void main (void)
{
   v_world_pos = u_model * vec4(a_position, 1.0);
   gl_Position = u_proj * u_view * v_world_pos;

    mat3 model_rot = mat3(normalize(u_model[0].xyz), normalize(u_model[1].xyz), normalize(u_model[2].xyz));
    v_normal = normalize(model_rot * a_normal);
    v_uv = a_uv;
    v_up = normalize(a_position);
    v_left = vec3(1, 0, 0);
    v_forward = cross(v_up, v_left);
    v_left = cross(v_forward, v_up);
}
