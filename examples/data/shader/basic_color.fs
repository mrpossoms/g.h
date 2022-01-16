#ifdef GL_ES
precision mediump float;
#endif

in vec4 v_color;

out vec4 color;

void main (void)
{
	color = v_color;
}
