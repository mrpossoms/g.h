#include <g.h>

using mat4 = xmath::mat<4,4>;

const std::string vs_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_src =
"varying vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";


struct my_core : public g::core
{
	g::gfx::shader basic_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::texture grid_tex;
	g::gfx::framebuffer fb;
	g::game::camera_perspective cam;

	virtual bool initialize()
	{
		std::cout << "initialize your game state here.\n";

		basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
											   .add_src<GL_FRAGMENT_SHADER>(fs_src)
											   .create();

		plane = g::gfx::mesh_factory::plane();

		fb = g::gfx::framebuffer_factory{512, 512}.color().create();
		grid_tex = g::gfx::texture_factory{}.from_png("data/tex/brick.color.png").create();
		
		cam.field_of_view = M_PI / 3;

		glDisable(GL_CULL_FACE);

		return true;
	}

	virtual void update(float dt)
	{
		t += dt;

		auto model = mat4::rotation({0, 0, 1}, t) * mat4::translation({0, 0, -1});

		// cam.d_pitch(dt);

		fb.bind_as_target();
		glClearColor(0, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		plane.using_shader(basic_shader)
		.set_camera(cam)
		["u_model"].mat4(model)
		["u_tex"].texture(grid_tex)
		.draw<GL_TRIANGLE_FAN>();
		fb.unbind_as_target();

		model = mat4::rotation({0, 1, 0}, t + M_PI) * mat4::translation({0, 0, -2});
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		plane.using_shader(basic_shader)
		.set_camera(cam)
		["u_model"].mat4(model)
		["u_tex"].texture(fb.color)
		.draw<GL_TRIANGLE_FAN>();
	}

	float t;
};


int main (int argc, const char* argv[])
{
	my_core core;

	core.start({ "05.offscreen_render", true, 512, 512 });

	return 0;
}
