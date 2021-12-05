#include "g.gfx.h"

using namespace g::gfx;
using namespace g::gfx::primative;


g::gfx::mesh<vertex::pos_uv_norm> g::gfx::primative::text::plane;


text::it::it(const std::string &str, g::gfx::font& f, size_t pos) : _font(f), _str(str)
{
	_pos = pos;
	_ctx.c = _str[_pos];
	_ctx.glyph = _font.char_map[_ctx.c];
	_ctx.pen = {0, -2};
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
	g::game::camera& cam,
	const mat<4, 4>& model)
{
	auto M = mat<4, 4>::translation({ 0, 0.5, 0 }) * model;
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
		auto p = ctx.pen + ctx.glyph.left_top + itr.kerning() + vec<2>{ctx.glyph.width, 0};
		auto glyph_model = mat<4, 4>::scale({-glyph.width, glyph.height, 1}) * mat<4, 4>::translation({p[0], p[1], 0});// * M;

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
			auto pos_aug = vec<4>{ vert.position[0], vert.position[1], vert.position[2], 1 };
			vert.position = (glyph_model.transpose() * pos_aug).slice<3>();
			vert.uv = (vec<2>{1.0, 1.0} - vert.uv) * (glyph.uv_bottom_right - glyph.uv_top_left) + glyph.uv_top_left;

			verts.push_back(vert);
		}

#ifdef DEBUG_TEXT_RENDERING
        debug::print{&cam}.color({1, 0, 0, 1}).model(M).point(ctx.pen);
        debug::print{&cam}.color({0, 1, 0, 1}).model(M).ray(ctx.pen, (p - ctx.pen));
#endif
	}

	plane.set_vertices(verts);
	plane.set_indices(inds);

	auto usage = plane.using_shader(shader)
	.set_camera(cam)
	["u_model"].mat4(M)
	["u_font_color"].vec4({1, 1, 1, 1})
	["u_texture"].texture(font.face);

	return usage;
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
