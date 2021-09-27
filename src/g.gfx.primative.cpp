#include "g.gfx.h"

using namespace g::gfx::primative;


text::it::it(const std::string &str, g::gfx::font& f, size_t pos) : _font(f), _str(str)
{
	_pos = pos;
	_ctx.glyph = _font.char_map[_str[_pos]];
	_ctx.pen = {0, -2};
}

void text::it::operator++()
{
	_ctx.pen += _ctx.glyph.advance * 2;
	_last_char = _str[_pos];
	_pos++;

	auto c = _str[_pos];

	if ('\n' == c)
	{
		_ctx.pen[0] = 0;
		_ctx.pen[1] -= 2;
		_pos++;
		c = _str[_pos];
	}

	_ctx.glyph = _font.char_map[c];
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

void text::draw(g::gfx::shader& shader,
	  const std::string& str,
      const g::game::camera& cam,
      const mat<4, 4>& model)
{
	auto end = it(str, font, str.length());
	for (auto itr = it(str, font, 0); itr != end; ++itr)
	{
		auto ctx = *itr;
		auto& glyph = ctx.glyph;//font.char_map[str[i]];

        auto p = ctx.pen + ctx.glyph.left_top + itr.kerning() + vec<2>{ctx.glyph.width, 0};

        auto glyph_model = mat<4, 4>::scale({-glyph.width, glyph.height, 1}) * mat<4, 4>::translation({p[0], p[1], 0}) * model;

        plane.using_shader(shader)
        .set_camera(cam)
        ["u_model"].mat4(glyph_model)
        ["u_font_color"].vec4({1, 1, 1, 1})
        ["u_uv_top_left"].vec2(glyph.uv_top_left)
        ["u_uv_bottom_right"].vec2(glyph.uv_bottom_right)
        ["u_texture"].texture(font.face)
        .draw_tri_fan();

        debug::print{&cam}.color({1, 0, 0, 1}).model(model).point(ctx.pen);
        debug::print{&cam}.color({0, 1, 0, 1}).model(model).ray(ctx.pen, (p - ctx.pen));
	}
}

void text::measure(const std::string& str, vec<2>& dims_out, vec<2>& offset_out)
{
	vec<2> min = {};
	vec<2> max = {};

	bool first = true;
	auto end = it(str, font, str.length() - 1);
	for (auto itr = it(str, font, 0); itr != end; ++itr)
	{
		auto ctx = *itr;
		auto p = ctx.pen + ctx.glyph.left_top + itr.kerning();
		min = min.take_min(p + vec<2>{-ctx.glyph.width, -ctx.glyph.height});
		max = max.take_max(p + vec<2>{ctx.glyph.width, ctx.glyph.height});
	
		if (first)
		{
			offset_out = p + vec<2>{-ctx.glyph.width, ctx.glyph.height};
			first = false;
		}
	}

	min *= vec<2>{1, -1};
	max *= vec<2>{1, -1};

	dims_out = (max - min);
}