#ifdef GL_ES
precision mediump float;
#endif

in vec3 a_position;
in vec3 a_normal;
in vec3 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main (void)
{
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
}
