#include "g.gfx.h"
#include <lodepng.h>
#include <algorithm>

#ifdef _WIN32
#include <corecrt_wio.h>
#endif

using namespace g::gfx;

std::unique_ptr<api::interface> api::instance;

bool g::gfx::has_graphics()
{
	return nullptr != g::gfx::GLFW_WIN;
}

size_t g::gfx::width()
{
	return api::instance->width();
}

size_t g::gfx::height()
{
	return api::instance->height();
}

float g::gfx::aspect()
{
	return api::instance->aspect();
}

float g::gfx::noise::perlin(const vec<3>& p, const std::vector<int8_t>& entropy)
{
	auto ent_data = entropy.data();
	auto ent_size = entropy.size();

	auto rand_grad = [&](vec<3, unsigned> a) -> vec<3>
	{
		const unsigned w = 8 * sizeof(int);
		const unsigned s = w / 2; // rotation width

		// unsigned a[3];
		// for (unsigned i = 3; i--;) { a[i] = index[i]; }

		for (unsigned i = 0; i < 3; i++)
		{
			a[i] *= *(const unsigned*)(ent_data + (a[i] % ent_size - sizeof(unsigned)));
			a[(i + 1) % 3] ^= a[i] << s | a[i] >> (w - s);
		}

		// a.v[0] *= *(const unsigned*)(ent_data + (a.v[0] % ent_size - sizeof(unsigned)));
		// a.v[(0 + 1) % 3] ^= a.v[0] << s | a.v[0] >> w - s;
		// a.v[1] *= *(const unsigned*)(ent_data + (a.v[1] % ent_size - sizeof(unsigned)));
		// a.v[(1 + 1) % 3] ^= a.v[1] << s | a.v[1] >> w - s;
		// a.v[2] *= *(const unsigned*)(ent_data + (a.v[2] % ent_size - sizeof(unsigned)));
		// a.v[(2 + 1) % 3] ^= a.v[2] << s | a.v[2] >> w - s;

		vec<3> grad = {
			(float)entropy[a.v[0] % ent_size],
			(float)entropy[a.v[1] % ent_size],
			(float)entropy[a.v[2] % ent_size],
		};

		return grad.unit();
	};

	/**
	 * 0-------1
	 * |       |
	 * x----*--x
	 * |       |
	 * 2-------3
	 *
	 * 00 -> 0
	 * 01 -> 2
	 * 10 -> 1
	 * 11 -> 3
	 *
	 *      0-------1
	 *     /       /|
	 *    /       / |
	 *   2-------3  |
	 *   |  4    |  5
	 *   |       | /
	 *   |       |/
	 *   6-------7
	 *
	 *0 000 -> 0
	 *1 001 -> 4
	 *2 010 -> 2
	 *3 011 -> 6
	 *4 100 -> 1
	 *5 101 -> 5
	 *6 110 -> 3
	 *7 111 -> 7
	 */

	const vec<3> bounds[2] = {
		p.floor(),
		p.floor() + vec<3>{1, 1, 1}
	};

	constexpr auto cn = 8;//1 << 3;

	auto w = p - bounds[0];
	float s[cn] = {};
	for (unsigned ci = 0; ci < cn; ci++)
	{
		vec<3> corner;

		// construct a corner based on the 
		// for (unsigned i = 0; i < 3; i++)
		// {
		// 	bool bit = (ci >> i) & 0x1;
		// 	corner[2 - i] = bit * bounds[1][2 - i] + (1 - bit) * bounds[0][2 - i];
		// }

		float bits[3] = {
			static_cast<float>((ci >> 0) & 0x1),
			static_cast<float>((ci >> 1) & 0x1),
			static_cast<float>((ci >> 2) & 0x1),
		};
		corner.v[2] = bits[0] * bounds[1].v[2] + (1 - bits[0]) * bounds[0].v[2];
		corner.v[1] = bits[1] * bounds[1].v[1] + (1 - bits[1]) * bounds[0].v[1];
		corner.v[0] = bits[2] * bounds[1].v[0] + (1 - bits[2]) * bounds[0].v[0];
		auto grad = rand_grad(corner.template cast<unsigned>());

		// std::cout << ci << ": " << corner.to_string() << " -> " << grad.to_string() << std::endl;

		s[ci] = grad.dot(corner - p);

		//sum += dots[ci];
	}

	auto za0 = s[0] * (1 - w[2]) + s[1] * w[2];
	auto za1 = s[2] * (1 - w[2]) + s[3] * w[2];

	auto ya0 = za0 * (1 - w[1]) + za1 * w[1];

	auto zb0 = s[4] * (1 - w[2]) + s[5] * w[2];
	auto zb1 = s[6] * (1 - w[2]) + s[7] * w[2];

	auto yb0 = zb0 * (1 - w[1]) + zb1 * w[1];

	return ya0 * (1 - w[0]) + yb0 * w[0];
}

float g::gfx::noise::value(const vec<3>& p, const std::vector<int8_t>& entropy)
{
	// auto ent_data = entropy.data();
	// auto ent_size = entropy.size();

	const vec<3> bounds[2] = {
		p.floor(),
		p.floor() + vec<3>{1, 1, 1}
	};

	constexpr auto cn = 1 << 3;

	float s[cn] = {};
	for (unsigned ci = 0; ci < cn; ci++)
	{

		// construct a corner based on the 
		// for (unsigned i = 0; i < 3; i++)
		// {
		// 	bool bit = (ci >> i) & 0x1;
		// 	corner[2 - i] = bit * bounds[1][2 - i] + (1 - bit) * bounds[0][2 - i];
		// }

		float bits[3] = {
			static_cast<float>((ci >> 0) & 0x1),
			static_cast<float>((ci >> 1) & 0x1),
			static_cast<float>((ci >> 2) & 0x1),
		};

		vec<3> corner = {
			bits[0] * bounds[1].v[2] + (1 - bits[0]) * bounds[0].v[2],
			bits[1] * bounds[1].v[1] + (1 - bits[1]) * bounds[0].v[1],
			bits[2] * bounds[1].v[0] + (1 - bits[2]) * bounds[0].v[0]
		};

		auto idx = corner.cast<int>();
		s[ci] = static_cast<float>(entropy[(idx[1] * 1024 + idx[0]) % entropy.size()]) / 256.f;

		//sum += dots[ci];
	}

	auto w = p - bounds[0];
	auto za0 = s[0] * (1 - w[2]) + s[1] * w[2];
	auto za1 = s[2] * (1 - w[2]) + s[3] * w[2];

	auto ya0 = za0 * (1 - w[1]) + za1 * w[1];

	auto zb0 = s[4] * (1 - w[2]) + s[5] * w[2];
	auto zb1 = s[6] * (1 - w[2]) + s[7] * w[2];

	auto yb0 = zb0 * (1 - w[1]) + zb1 * w[1];

	return ya0 * (1 - w[0]) + yb0 * w[0];
}

void texture::release_bitmap()
{
	if (data)
	{
		free(data);
		data = nullptr;
	}
}

void texture::create(GLenum texture_type)
{
	type = texture_type;
	glGenTextures(1, &this->hnd);
	//assert(gl_get_error());
}

void texture::destroy()
{
	glDeleteTextures(1, &hnd);
	release_bitmap();
}

void texture::set_pixels(size_t w, size_t h, size_t d, unsigned char* data, GLenum color_type, GLenum storage)
{
	size[0] = w;
	size[1] = h;
	size[2] = d;
	this->data = data;

	switch(storage)
	{
		case GL_UNSIGNED_BYTE:
		case GL_BYTE:
			this->bytes_per_component = 1;
			break;

		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
			this->bytes_per_component = 2;
			break;

		case GL_UNSIGNED_INT:
		case GL_INT:
		case GL_FLOAT:
			this->bytes_per_component = 4;
			break;
	}

	switch(color_type)
	{
		case GL_RED:
			this->component_count = 1;
			break;
		case GL_RG:
			this->component_count = 2;
			break;
		case GL_RGB:
			this->component_count = 3;
			break;
		case GL_RGBA:
			this->component_count = 4;
			break;
	}

	if (h > 1 && d > 1)
	{
		type = GL_TEXTURE_3D;
		glTexImage3D(GL_TEXTURE_3D, 0, color_type, size[0], size[1], size[2], 0, color_type, storage, data);
	}
	else if (h >= 1)
	{
		type = GL_TEXTURE_2D;
		if (storage == GL_FLOAT && color_type == GL_RGBA)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size[0], size[1], 0, color_type, storage, data);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, color_type, size[0], size[1], 0, color_type, storage, data);
		}
	}
}

size_t texture::bytes() const
{
	auto pixels = size[0] * size[1] * size[2];
	return pixels * bytes_per_component * component_count;
}


void texture::get_pixels(unsigned char** data_out, size_t& data_out_size) const
{
	data_out_size = size[0] * size[1] * size[2];
	*data_out = new unsigned char[data_out_size];

	this->bind();

}


void texture::to_disk(const std::string& path) const
{

}

void texture::bind() const { glBindTexture(type, hnd); }

void texture::unbind() const { glBindTexture(type, 0); }

texture_factory::texture_factory(unsigned w, unsigned h)
{
	data = nullptr;
	texture_type = GL_TEXTURE_2D;
	size[0] = w;
	size[1] = h;
	size[2] = 1;
}

texture_factory::texture_factory(unsigned w, unsigned h, unsigned d)
{
	data = nullptr;
	texture_type = GL_TEXTURE_3D;
	size[0] = w;
	size[1] = h;
	size[2] = d;
}

texture_factory::texture_factory(texture* existing_texture)
{
	existing = existing_texture;
	data = existing->data;
	texture_type = existing->type;
	size[0] = existing->size[0];
	size[1] = existing->size[1];
	size[2] = existing->size[2];
}

void texture_factory::abort(std::string message)
{
	std::cerr << message << std::endl;
	exit(-1);
}

texture_factory& texture_factory::from_png(const std::string& path)
{
	std::cerr << "loading texture '" <<  path << "'... ";

	if (nullptr != data)
	{
		delete data;
		data = nullptr;
	}

	// LodePNGColorType colortype;
	// unsigned bitdepth;

	// TODO: use a more robust lodepng_decode call which can handle textures of various channels and bit depths
	// unsigned error = lodepng_decode_file((unsigned char**)&data, size[0], size[1], path.c_str());
	unsigned error = lodepng_decode32_file((unsigned char**)&data, size + 0, size + 1, path.c_str());


	// If there's an error, display it.
	if(error != 0)
	{
	  std::cout << G_TERM_RED "[lodepng::decode] error " << error << ": " << lodepng_error_text(error) << G_TERM_COLOR_OFF << std::endl;
	  exit(-1);
	}

	// a png is a 2D matrix of pixels
	texture_type = GL_TEXTURE_2D;
	color_type = GL_RGBA;
	storage_type = GL_UNSIGNED_BYTE;

	// switch(colortype)
	// {
	// 	case LCT_GREY:
	// 		color_type = GL_R;
	// 		break;
	// 	case LCT_GREY_ALPHA:
	// 		color_type = GL_RG;
	// 		break;
	// 	case LCT_RGB:
	// 		color_type = GL_RGB;
	// 		break;
	// 	case LCT_RGBA:
	// 		color_type = GL_RGBA;
	// 		break;
	// 	default:
	// 		std::cerr << G_TERM_RED << "Invalid component count: '" <<  component_count << G_TERM_COLOR_OFF << std::endl;
	// }

	std::cerr << G_TERM_GREEN "OK" G_TERM_COLOR_OFF << std::endl;

	return *this;
}

texture_factory& texture_factory::to_png(const std::string& path)
{
	std::cerr << "writing texture '" <<  path << "'... ";

	LodePNGColorType color_type = LCT_RGB;

	switch(component_count)
	{
		case 1:
			color_type = LCT_GREY;
			break;
		case 2:
			color_type = LCT_GREY_ALPHA;
			break;
		case 3:
			color_type = LCT_RGB;
			break;
		case 4:
			color_type = LCT_RGBA;
			break;
		default:
			std::cerr << G_TERM_RED << "Invalid component count: '" <<  component_count << G_TERM_COLOR_OFF << std::endl;
	}

	auto error = lodepng_encode_file(
		path.c_str(),
		(const unsigned char*)data,
		size[0], size[1],
		color_type,
		bytes_per_component * 8
	);

	// If there's an error, display it.
	if(error != 0)
	{
	  std::cout << G_TERM_RED "[lodepng::encode] error " << error << ": " << lodepng_error_text(error) << G_TERM_COLOR_OFF << std::endl;
	  exit(-1);
	}

	std::cerr << G_TERM_GREEN "OK" G_TERM_COLOR_OFF << std::endl;

	return *this;
}

texture_factory& texture_factory::type(GLenum t)
{
	storage_type = t;

	switch(t)
	{
		case GL_UNSIGNED_BYTE:
		case GL_BYTE:
			bytes_per_component = 1;
			break;

		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
			bytes_per_component = 2;
			break;

		case GL_UNSIGNED_INT:
		case GL_INT:
		case GL_FLOAT:
			bytes_per_component = 4;
			break;
	}

	return *this;
}

texture_factory& texture_factory::color()
{
	color_type = GL_RGBA;
	storage_type = GL_UNSIGNED_BYTE;
	return *this;
}

texture_factory& texture_factory::components(unsigned count)
{
	switch(component_count = count)
	{
		case 1:
			color_type = GL_R;
			break;
		case 2:
			color_type = GL_RG;
			break;
		case 3:
			color_type = GL_RGB;
			break;
		case 4:
			color_type = GL_RGBA;
			break;
		default:
			std::cerr << "Invalid number of components: " << count << std::endl;
	}

	return *this;
}

texture_factory& texture_factory::depth()
{
	color_type = GL_DEPTH_COMPONENT;
	storage_type = GL_UNSIGNED_SHORT;
	return *this;
}

texture_factory& texture_factory::pixelated()
{
	min_filter = mag_filter = GL_NEAREST;
	return *this;
}

texture_factory& texture_factory::smooth()
{
	min_filter = mag_filter = GL_LINEAR;
	return *this;
}

texture_factory& texture_factory::clamped()
{
	wrap_s = wrap_t = wrap_r = GL_CLAMP_TO_EDGE;
	return *this;
}

texture_factory& texture_factory::clamped_to_border()
{
	wrap_s = wrap_t = wrap_r = GL_CLAMP_TO_BORDER;
	return *this;
}

texture_factory& texture_factory::repeating()
{
	wrap_s = wrap_t = wrap_r = GL_REPEAT;
	return *this;
}


texture_factory& texture_factory::fill(std::function<void(int x, int y, int z, unsigned char* pixel)> filler)
{
	// void* pixels;

	auto bytes_per_textel = bytes_per_component * component_count;
	auto textels_per_row = size[2];
	auto textels_per_plane = size[1] * size[2];
	data = new unsigned char[bytes_per_textel * size[0] * size[1] * size[2]];

	for (unsigned i = 0; i < size[0]; i++)
	{
		for (unsigned j = 0; j < size[1]; j++)
		{
			for (unsigned k = 0; k < size[2]; k++)
			{
				// unsigned vi = i * textels_per_plane + ((((j * size[2]) + k) * bytes_per_pixel));
				// unsigned vi = (i * textels_per_plane) + (j * bytes_per_row) + (k * bytes_per_pixel);
				// filler(i, j, k, data + vi);

	            unsigned vi = ((i * textels_per_plane) + (j * textels_per_row) + k) * bytes_per_textel;

	            filler(i, j, k, data + vi);
			}
		}
	}

	return *this;
}

texture_factory& texture_factory::fill(unsigned char* buffer)
{
	data = buffer;

	return *this;
}


texture texture_factory::create()
{
	texture out;

	if (existing)
	{
		out = *existing;
	}
	else
	{
		out.create(texture_type);		
	}

	out.bind();

	//assert(gl_get_error());
	out.set_pixels(size[0], size[1], size[2], data, color_type, storage_type);
	//assert(gl_get_error());


	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_t);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_R, wrap_r);
	glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, min_filter);
	//assert(gl_get_error());
	// glGenerateMipmap(GL_TEXTURE_2D);
	//assert(gl_get_error());

	return out;
}


framebuffer_factory::framebuffer_factory(unsigned w, unsigned h)
{
	size[0] = w;
	size[1] = h;
}

framebuffer_factory::framebuffer_factory(texture& dst)
{
	color_tex = dst;
	size[0] = color_tex.size[0];
	size[1] = color_tex.size[1];
}

framebuffer_factory& framebuffer_factory::color()
{
	color_tex = texture_factory{ size[0], size[1] }.color().clamped().smooth().create();
	return *this;
}

framebuffer_factory& framebuffer_factory::depth()
{
	depth_tex = texture_factory{ size[0], size[1] }.depth().clamped().smooth().create();
	return *this;
}

framebuffer_factory& framebuffer_factory::shadow_map()
{
	return depth();
}

framebuffer framebuffer_factory::create()
{
	framebuffer fb;

	fb.size[0] = size[0];
	fb.size[1] = size[1];
	fb.color = color_tex;
	fb.depth = depth_tex;
	glGenFramebuffers(1, &fb.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	//assert(gl_get_error());

	if (color_tex.hnd != (GLuint)-1)
	{
		color_tex.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.hnd, 0);
	}

	if (depth_tex.hnd != (GLuint)-1)
	{
		depth_tex.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.hnd, 0);
	}

	auto fb_stat = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	switch(fb_stat)
	{
		case GL_FRAMEBUFFER_UNDEFINED:
			std::cerr << "Target is the default framebuffer, but the default framebuffer does not exist.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cerr << "One or more of the framebuffer attachment points are framebuffer incomplete.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cerr << "The framebuffer does not have at least one image attached to it.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			std::cerr << "the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAWBUFFERi.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			std::cerr << "GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.\n";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cerr << "The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			std::cerr << "the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			std::cerr << "framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.\n";
			break;
		default:
			std::cerr << "An unknown error ocurred.\n";
			break;
	}

	assert(fb_stat == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fb;
}



shader& shader::bind() { glUseProgram(program); return *this; }

void shader::destroy()
{	
	glDeleteProgram(program);
}

shader::usage::usage (shader* ref, size_t verts, size_t inds) : shader_ref(ref)
{
	vertices = verts;
	indices = inds;
	texture_unit = 0;
}


shader::usage shader::usage::set_camera(g::game::camera& cam)
{
	//assert(gl_get_error());
	this->set_uniform("u_view").mat4(cam.view());
	this->set_uniform("u_proj").mat4(cam.projection().transpose());
	//assert(gl_get_error());
	return *this;
}


shader::usage shader::usage::set_sprite(const g::gfx::sprite::instance& sprite)
{
	this->set_uniform("u_sprite_sheet").texture(*sprite.sheet->texture);
	this->set_uniform("u_sprite_sheet_size").vec2(sprite.sheet->sheet_size);
	this->set_uniform("u_sprite_sheet_frame_pos").vec2(sprite.current_frame().position);
	this->set_uniform("u_sprite_sheet_frame_size").vec2(sprite.current_frame().size);

	return *this;
}


shader::uniform_usage shader::usage::set_uniform(const std::string& name)
{
	if (nullptr == shader_ref) { return {*this, 0xffffffff}; }

	GLint loc;
	auto it = shader_ref->uni_locs.find(name);
	if (it == shader_ref->uni_locs.end())
	{
		loc = glGetUniformLocation(shader_ref->program, name.c_str());

		if (loc < 0)
		{
			// TODO: handle the missing uniform better
			std::cerr << "uniform '" << name << "' doesn't exist\n";
			shader_ref->uni_locs[name] = loc;
		}
	}
	else
	{
		loc = (*it).second;
	}

	return uniform_usage(*this, loc);
}

shader::uniform_usage shader::usage::operator[](const std::string& name)
{
	return set_uniform(name);
}


shader::uniform_usage::uniform_usage(shader::usage& parent, GLuint loc) : parent_usage(parent) { uni_loc = loc; }

shader::usage shader::uniform_usage::mat4 (const mat<4, 4>& m)
{
	glUniformMatrix4fv(uni_loc, 1, true, m.ptr());

	return parent_usage;
}

shader::usage shader::uniform_usage::mat3 (const mat<3, 3>& m)
{
	glUniformMatrix3fv(uni_loc, 1, true, m.ptr());

	return parent_usage;
}

shader::usage shader::uniform_usage::vec2 (const vec<2>& v)
{
	glUniform2fv(uni_loc, 1, v.v);

	return parent_usage;
}

shader::usage shader::uniform_usage::vec2n (const vec<2>* v, size_t count)
{
	glUniform2fv(uni_loc, count, (const float*)v);

	return parent_usage;
}

shader::usage shader::uniform_usage::vec3 (const vec<3>& v)
{
	glUniform3fv(uni_loc, 1, v.v);

	return parent_usage;
}

shader::usage shader::uniform_usage::vec3n (const vec<3>* v, size_t count)
{
	glUniform3fv(uni_loc, count, (const float *)v);

	return parent_usage;
}

shader::usage shader::uniform_usage::vec4(const vec<4>& v)
{
	glUniform4fv(uni_loc, 1, v.v);

	return parent_usage;
}

shader::usage shader::uniform_usage::flt (float f)
{
	glUniform1f(uni_loc, f);

	return parent_usage;
}

shader::usage shader::uniform_usage::fltn (float* f, size_t count)
{
	glUniform1fv(uni_loc, count, (const float *)f);

	return parent_usage;
}

shader::usage shader::uniform_usage::int1(const int i)
{
	glUniform1i(uni_loc, i);
	return parent_usage;
}

shader::usage shader::uniform_usage::texture(const g::gfx::texture& tex)
{
	glActiveTexture(GL_TEXTURE0 + parent_usage.texture_unit);
	tex.bind();
	glUniform1i(uni_loc, parent_usage.texture_unit);
	parent_usage.texture_unit++;
	return parent_usage;
}

#ifdef __EMSCRIPTEN__
	std::string shader_factory::shader_header = "#version 300 es\n";
#else
	std::string shader_factory::shader_header = "#version 410\n";
#endif

	std::string shader_factory::shader_path;

GLuint shader_factory::compile_shader (GLenum type, const GLchar* src, GLsizei len)
{
	// Create the GL shader and attempt to compile it
	auto shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, &len);
	glCompileShader(shader);

	//assert(gl_get_error());

	// Check the compilation status
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cerr << G_TERM_RED << "FAILED " << status << G_TERM_COLOR_OFF << std::endl;
		std::cerr << G_TERM_YELLOW << src << G_TERM_COLOR_OFF << std::endl;
	}
	//assert(gl_get_error());

	// Print the compilation log if there's anything in there
	GLint log_length;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		GLchar *log_str = (GLchar *)malloc(log_length);
		glGetShaderInfoLog(shader, log_length, &log_length, log_str);
		std::cerr << G_TERM_RED << "Shader compile log: " << log_length << std::endl << log_str << G_TERM_COLOR_OFF << std::endl;
		free(log_str);
	}
	//assert(gl_get_error());

	// treat all shader compilation failure as fatal
	if (status == GL_FALSE)
	{
		glDeleteShader(shader);
		exit(-2);
	}

	std::cerr << G_TERM_GREEN << "OK" << G_TERM_COLOR_OFF << std::endl;

	return shader;
}


shader shader_factory::create()
{
	GLint status;
	shader out;
	out.program = glCreateProgram();

	for (auto shader : shaders)
	{
		glAttachShader(out.program, shader.second);
	}

	//assert(gl_get_error());
	glLinkProgram(out.program);

	glGetProgramiv(out.program, GL_LINK_STATUS, &status);
	if (status == 0)
	{
		GLint log_length;
		glGetProgramiv(out.program, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			GLchar *log_str = (GLchar *)malloc(log_length);
			glGetProgramInfoLog(out.program, log_length, &log_length, log_str);
			std::cerr << "Shader link log: " << log_length << std::endl << log_str << std::endl;
			::write(1, log_str, log_length);
			free(log_str);
		}
		exit(-1);
	}

	//assert(gl_get_error());

	// Detach all
	for (auto shader : shaders)
	{
		glDetachShader(out.program, shader.second);
	}

	return out;
}


#include <ft2build.h>
#include FT_FREETYPE_H

font font_factory::from_true_type(const std::string& path, unsigned point)
{
	g::gfx::font font;

	FT_Library  library;   /* handle to library     */
	FT_Face     face;      /* handle to face object */

	auto err_init = FT_Init_FreeType( &library );
	if (err_init)
	{
		std::cerr << G_TERM_RED "font_factory: " <<
		"FT_Init_FreeType() failed "
		<< G_TERM_COLOR_OFF << std::endl;
		assert(false);
	}

	auto err_load = FT_New_Face(library, path.c_str(), 0, &face);
	if (err_load == FT_Err_Unknown_File_Format)
	{
		std::cerr << G_TERM_RED "font_factory: " <<
		"the font file could be opened and read, but it appears "
		"that its font format is unsupported"
		<< G_TERM_COLOR_OFF << std::endl;
	}
	else if (err_load)
	{
		std::cerr << G_TERM_RED "font_factory: " <<
		"another error code means that the font file could not "
		"be opened or read, or that it is broken..."
		<< G_TERM_COLOR_OFF << std::endl;
	}

	assert(!err_load);

	size_t pix_per_glyph = point;
	if (FT_Set_Pixel_Sizes(face,   /* handle to face object */
                           0,      /* pixel_width           */
						   pix_per_glyph))   /* pixel_height          */
	{
	/*
	Usually, an error occurs with a fixed-size font format (like FNT or PCF)
	when trying to set the pixel size to a value that is not listed in the
	face->fixed_sizes array.
	*/
		assert(false);
	}

	unsigned rows = sqrt(256), cols = sqrt(256);
	unsigned row_pix = rows * pix_per_glyph;
	unsigned col_pix = cols * pix_per_glyph;
	unsigned bytes_per_pixel = 0;
	unsigned bytes_per_map_row = 0;
	unsigned map_size = 0;
	unsigned char* map_buffer = nullptr;

	for (unsigned ci = 1; ci < 256; ci++)
	{
		auto glyph_idx = FT_Get_Char_Index(face, ci);
		if (FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT )) { continue; }

		font.kerning_map.insert({(unsigned char)ci, {}}); // add a new map
		for (unsigned cii = 1; cii < 256; cii++)
		{
			auto adj_glyph_idx = FT_Get_Char_Index(face, cii);

			FT_Vector kern;
			if (FT_Get_Kerning(face, glyph_idx, adj_glyph_idx, FT_KERNING_DEFAULT, &kern))
			{
				continue;
			}

			if (kern.x == 0 && kern.y == 0)
			{ // there's no point in storing kerning for pairs which have no adjustment
				continue;
			}

			font.kerning_map[ci].insert({(unsigned char)cii, {kern.x / (float)point, kern.y / (float)point}});

		}

		auto slot = face->glyph;
		if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL )) { continue; /* skip on err */ }

		// allocate the full character set buffer
		if (nullptr == map_buffer)
		{
			bytes_per_pixel = slot->bitmap.pitch / slot->bitmap.width;
			bytes_per_map_row = col_pix * bytes_per_pixel;
			map_size = row_pix * bytes_per_map_row;
			map_buffer = new unsigned char[map_size];
		}

		auto glyph_row = ci / cols;
		auto glyph_col = ci % cols;

		auto map_row = glyph_row * pix_per_glyph;

		// printf("VV char '%c'\n", ci);
		for (unsigned r = 0; r < slot->bitmap.rows; r++)
		{
			if (slot->bitmap.rows > pix_per_glyph) { break; }
			auto row_off = (r + map_row) * bytes_per_map_row;
			auto col_off = (glyph_col * pix_per_glyph) * bytes_per_pixel;
			assert(row_off + col_off < map_size);
			auto map_buf_ptr = &map_buffer[row_off + col_off];
			memcpy(map_buf_ptr, &slot->bitmap.buffer[r * slot->bitmap.pitch], slot->bitmap.pitch);
		}

		// compute uvs for each glyph
		auto uv_upper_left = vec<2>{(float)glyph_col, (float)(glyph_row)} * (pix_per_glyph / (float)row_pix);
		auto uv_lower_right = uv_upper_left + vec<2>{ slot->bitmap.width / (float)row_pix, slot->bitmap.rows /(float)row_pix };
		// auto uv_lower_right = vec<2>{glyph_col + 1.f, (glyph_row + 1)} * (pix_per_glyph / (float)row_pix);
		font.char_map.insert({
			(unsigned char)ci, {
				{ uv_lower_right[0], uv_upper_left[1] },
				{ uv_upper_left[0], uv_lower_right[1] },
				slot->bitmap.width / (float)point,
				slot->bitmap.rows / (float)point,
				{ (float)-slot->bitmap_left / (float)point, (float)slot->bitmap_top / (float)point },
				{ (float)(slot->advance.x / 64.f) / (float)point, (float)(slot->advance.y / 64.f) / (float)point },
			}
		});
	}

	FT_Done_Face    ( face );
	FT_Done_FreeType( library );

	// for(unsigned r = 0; r < row_pix; r+=1)
	// {
	// 	for(unsigned c = 0; c < col_pix; c+=1)
	// 	{
	// 		putchar(buffer[(r * bytes_per_map_row) + (c * 1)] > 0 ? 'x' : ' ');
	// 	}
	// 	putchar('\n');
	// }
	// std::cerr<<"done\n";

	// using RGBA is totally excessive, but webgl
	// seems to have issues with just GL_RED
	font.face = texture_factory{col_pix, row_pix}
	.type(GL_UNSIGNED_BYTE)
	.components(4)
	.fill([&](int x, int y, int z, unsigned char* pixel){
		pixel[0] = pixel[1] = pixel[2] = 0xff;
		pixel[3] = map_buffer[(x * row_pix) + y];
	}).pixelated().clamped().create();

	font.point = point;

	delete[] map_buffer;

	return font;
}
