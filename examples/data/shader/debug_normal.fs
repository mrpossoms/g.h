#version 410
#ifdef GL_ES
precision mediump float;
#endif

in vec3 v_normal;

out vec4 color;

void main (void)
{
    color = vec4(v_normal * 0.5 + 0.5, 1.0);
}
