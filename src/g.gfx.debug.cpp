#include "g.gfx.h"

using namespace g::gfx;

static g::gfx::shader debug_shader;
static g::gfx::mesh<vertex::pos> debug_mesh;

const std::string vs_dbg_src =
"#version 410\n"
"in vec3 a_position;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"uniform mat4 u_model;"
"void main (void) {"
"gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);"
"}";

const std::string fs_dbg_src =
"#version 410\n"
"uniform vec4 u_color;"
"out vec4 color;"
"void main (void) {"
"color = u_color;"
"}";


debug::print::print(const g::game::camera* cam)
{
	if (!debug_shader.is_initialized())
	{
		debug_shader = shader_factory{}
			.add_src<GL_VERTEX_SHADER>(vs_dbg_src)
			.add_src<GL_FRAGMENT_SHADER>(fs_dbg_src)
			.create();
	}


	if (!debug_mesh.is_initialized())
	{
		debug_mesh = mesh_factory::empty_mesh<vertex::pos>();
	}

	cur_cam = cam;
}

debug::print& debug::print::color(const vec<4>& c)
{
	cur_color = c;
	return *this;
}

debug::print& debug::print::model(const mat<4, 4>& m)
{
	cur_model = m;
	return *this;
}

void debug::print::ray(const vec<2>& o, const vec<2>& d)
{
	ray(vec<3>{o[0], o[1], 0}, vec<3>{d[0], d[1], 0});
}

void debug::print::ray(const vec<3>& o, const vec<3>& d)
{
	vertex::pos verts[2] = {
		{ o },
		{ o + d},
	};

	assert(gl_get_error());
	debug_mesh.set_vertices(verts, 2);
	assert(gl_get_error());
	debug_mesh.using_shader(debug_shader)
	          .set_camera(*cur_cam)
	          .set_uniform("u_color").vec4(cur_color)
	          .set_uniform("u_model").mat4(cur_model)
	          .draw<GL_LINES>();
	assert(gl_get_error());
}

void debug::print::point(const vec<2>& o)
{
	point(vec<3>{o[0], o[1], 0});
}

void debug::print::point(const vec<3>& o)
{
	vertex::pos verts[1] = {
		{ o },
	};

	assert(gl_get_error());
	debug_mesh.set_vertices(verts, 1);
	assert(gl_get_error());
	debug_mesh.using_shader(debug_shader)
	          .set_camera(*cur_cam)
	          .set_uniform("u_color").vec4(cur_color)
	          .set_uniform("u_model").mat4(cur_model)
	          .draw<GL_POINTS>();
	assert(gl_get_error());
}