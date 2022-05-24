#include "g.gfx.h"

using namespace g::gfx;
using namespace g::gfx::primative;

g::gfx::mesh<vertex::pos_uv_norm> g::gfx::primative::text::plane;


text::it::it(const std::string &str, g::gfx::font& f, size_t pos) : _font(f), _str(str)
{
	_pos = pos;
	_ctx.c = _str[_pos];
	_ctx.glyph = _font.char_map[_ctx.c];
	_ctx.pen = {0, 0};
}

void text::it::operator++()
{
	_ctx.pen += _ctx.glyph.advance * 2;
	_last_char = _str[_pos];
	_pos++;

	_ctx.c = _str[_pos];

	if ('\n' == _ctx.c)
	{
		_ctx.pen[0] = 0;
		_ctx.pen[1] -= 2;
		_pos++;
		_ctx.c = _str[_pos];
	}

	_ctx.glyph = _font.char_map[_ctx.c];
}

vec<2> text::it::kerning() { return _font.kerning_map[_last_char][_str[_pos]]; }

bool text::it::operator!=(it &i) {return _pos != i._pos || _str != i._str; }

text::it::ctx text::it::operator*() { return _ctx; }



text::text(g::gfx::font& f) : font(f)
{
	if (!plane.is_initialized())
	{
		plane = g::gfx::mesh_factory{}.plane();
	}
}

shader::usage text::using_shader(g::gfx::shader& shader,
                                 const std::string& str,
                                 std::function<void(g::gfx::shader::usage&)> shader_config)
{

	static std::vector<vertex::pos_uv_norm> verts;
	static std::vector<uint32_t> inds;

	const vertex::pos_uv_norm tri_quad[] = {
		{{-1,-1, 0}, {1, 0}, {0, 0, 1}},
		{{ 1,-1, 0}, {0, 0}, {0, 0, 1}},
		{{ 1, 1, 0}, {0, 1}, {0, 0, 1}},
		{{-1, 1, 0}, {1, 1}, {0, 0, 1}},
	};

	verts.clear();
	inds.clear();
	auto end = it(str, font, str.length());
	for (auto itr = it(str, font, 0); itr != end; ++itr)
	{
		auto ctx = *itr;
		auto& glyph = ctx.glyph;//font.char_map[str[i]];
		auto p = (ctx.pen + ctx.glyph.left_top + itr.kerning() + vec<2>{ctx.glyph.width, 0}) * font.point;

		vec<3> glyph_scale({-glyph.width * font.point, glyph.height * font.point, 1});
		vec<3> glyph_pos({p[0], p[1], 0});

		auto ct = verts.size();
		inds.push_back(ct + 2);
		inds.push_back(ct + 3);
		inds.push_back(ct + 0); 
		inds.push_back(ct + 1);
		inds.push_back(ct + 2);
		inds.push_back(ct + 0);

		for (unsigned i = 0; i < 4; i++)
		{
			auto vert = tri_quad[i];
			vert.position = (vert.position * glyph_scale) + glyph_pos;
			vert.uv = (vec<2>{1.0, 1.0} - vert.uv) * (glyph.uv_bottom_right - glyph.uv_top_left) + glyph.uv_top_left;

			verts.push_back(vert);
		}
	}

	plane.set_vertices(verts);
	plane.set_indices(inds);

	auto usage = plane.using_shader(shader)
	                   ["u_texture"].texture(font.face);

	if (shader_config)
	{
		shader_config(usage);
	}	

	return usage;
}

shader::usage text::using_shader (g::gfx::shader& shader,
	const std::string& str,
	g::game::camera& cam,
	const mat<4, 4>& model)
{
	return using_shader(shader, str, [&](g::gfx::shader::usage& usage) {
		usage.set_camera(cam)
	              ["u_model"].mat4(mat<4, 4>::translation({ 0, 0.5, 0 }) * model)
	              ["u_font_color"].vec4({1, 1, 1, 1})
	              ["u_texture"].texture(font.face);
	});
}

void text::draw(g::gfx::shader& shader,
	  const std::string& str,
      g::game::camera& cam,
      const mat<4, 4>& model)
{
	using_shader(shader, str, cam, model).draw<GL_TRIANGLES>();//.draw_tri_fan();
}


void text::measure(const std::string& str, vec<2>& dims_out, vec<2>& offset_out)
{
	vec<2> min = {};
	vec<2> max = {};

	bool first = true;
	auto end = it(str, font, str.length());
	for (auto itr = it(str, font, 0); itr != end; ++itr)
	{
		auto ctx = *itr;
		auto p = ctx.pen + ctx.glyph.left_top + itr.kerning();
		min = min.take_min(p + vec<2>{-ctx.glyph.width, 0});
		max = max.take_max(p + vec<2>{ctx.glyph.width, ctx.glyph.height});

		if (first)
		{
			offset_out = p + vec<2>{0, ctx.glyph.height};
			first = false;
		}
	}

	offset_out *= vec<2>{-1, -1};
	min *= vec<2>{1, -1};
	max *= vec<2>{1, -1};

	dims_out = (max - min);
}
