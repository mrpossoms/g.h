#include <g.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

const std::string vs_src =
"in vec3 a_position;"
"in vec2 a_uv;"
"in vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_proj;"
"out vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_model * vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_src =
"in vec2 v_uv;"
"uniform sampler2D u_tex;"
"out vec4 color;"
"void main (void) {"
"color = texture(u_tex, v_uv);"
"}";


struct my_core : public g::core
{
	g::gfx::shader basic_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::texture grid_tex;
	g::asset::store assets;

	virtual bool initialize()
	{
		std::cout << "initialize your game state here.\n";

		basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
											   .add_src<GL_FRAGMENT_SHADER>(fs_src)
											   .create();

		plane = g::gfx::mesh_factory::plane();

		std::vector<int8_t> v;
		for (unsigned i = 1024; i--;)
		{
			v.push_back(rand() % 255 - 128);
		}

		glDisable(GL_CULL_FACE);
		grid_tex = g::gfx::texture_factory{256, 256}
		.type(GL_UNSIGNED_BYTE)
		.components(1)
		.fill([&](int x, int y, int z, unsigned char* pixel) {
			*pixel = (unsigned char)(g::gfx::perlin<2>({x / 10.f, y / 10.f}, v) * 255);
		})
		.create()
		//.from_png("data/tex/brick.color.png").create();

		return true;
	}

	virtual void update(float dt)
	{
		glClearColor(0, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		t += dt;

		auto model = mat4::rotation({0, 1, 0}, t + M_PI) * mat4::translation({0, 0, -1});
		auto proj = mat4::perspective(0.1, 10, M_PI / 2, g::gfx::aspect());

		plane.using_shader(basic_shader)
		["u_model"].mat4(model)
		["u_proj"].mat4(proj)
		["u_tex"].texture(assets.tex("brick.color.png"))
		.draw<GL_TRIANGLE_FAN>();
	}

	float t;
};

my_core core;

// void main_loop() { core.tick(); }

int main (int argc, const char* argv[])
{

// #ifdef __EMSCRIPTEN__
// 	core.running = false;
// 	core.start({ "04.basic_draw", true, 512, 512 });
// 	emscripten_set_main_loop(main_loop, 144, 1);
// #else
	core.start({ "04.basic_draw", true, 512, 512 });
// #endif

	return 0;
}
