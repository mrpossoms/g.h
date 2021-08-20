#include "g.gfx.h"
#include <lodepng.h>

#ifdef _WIN32
#include <corecrt_wio.h>
#endif

using namespace g::gfx;



size_t g::gfx::width()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width;
}

size_t g::gfx::height()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return height;
}

float g::gfx::aspect()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width / (float)height;
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
	assert(gl_get_error());
}

void texture::destroy()
{
	glDeleteTextures(1, &hnd);
	release_bitmap();
}

void texture::set_pixels(size_t w, size_t h, size_t d, char* data, GLenum format, GLenum storage)
{
	size[0] = w;
	size[1] = h;
	size[2] = d;
	this->data = data;

	if (h > 1 && d > 1)
	{
		type = GL_TEXTURE_3D;
		glTexImage3D(GL_TEXTURE_3D, 0, format, size[0], size[1], size[2], 0, format, storage, data);
	}
	else if (h > 1)
	{
		type = GL_TEXTURE_2D;
		glTexImage2D(GL_TEXTURE_2D, 0, format, size[0], size[1], 0, format, storage, data);
	}
}

void texture::bind() const { glBindTexture(type, hnd); }



texture_factory::texture_factory(unsigned w, unsigned h)
{
	data = nullptr;
	texture_type = GL_TEXTURE_2D;
	size[0] = w;
	size[1] = h;
}

texture_factory::texture_factory(unsigned w, unsigned h, unsigned d)
{
	data = nullptr;
	texture_type = GL_TEXTURE_3D;
	size[0] = w;
	size[1] = h;
	size[2] = d;
}

void texture_factory::abort(std::string message)
{
	std::cerr << message << std::endl;
	exit(-1);
}

texture_factory& texture_factory::from_png(const std::string& path)
{
	std::cerr << "loading texture '" <<  path << "'... ";

	unsigned width, height;
	unsigned error = lodepng_decode32_file((unsigned char**)&data, size + 0, size + 1, path.c_str());

	// If there's an error, display it.
	if(error != 0)
	{
	  std::cout << G_TERM_RED "[lodepng::decode] error " << error << ": " << lodepng_error_text(error) << G_TERM_COLOR_OFF << std::endl;
	  exit(-1);
	}

	// a png is a 2D matrix of pixels
	texture_type = GL_TEXTURE_2D;

	// TODO: change. this is hacky, I don't love it, but it sure is simple
	color_type = GL_RGBA;
/*
	switch (png_color_type) {
		case PNG_COLOR_TYPE_RGBA:
			color_type = GL_RGBA;
			break;
		case PNG_COLOR_TYPE_PALETTE:
		case PNG_COLOR_TYPE_RGB:
			color_type = GL_RGB;
			break;
	}
*/
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
			color_type = GL_RED;
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


texture texture_factory::create()
{
	texture out;


	out.create(texture_type);
	out.bind();

	assert(gl_get_error());
	out.set_pixels(size[0], size[1], size[2], data, color_type, storage_type);
	assert(gl_get_error());


	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_t);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_R, wrap_r);
	glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, min_filter);
	assert(gl_get_error());
	// glGenerateMipmap(GL_TEXTURE_2D);
	assert(gl_get_error());

	return out;
}

framebuffer_factory::framebuffer_factory(unsigned w, unsigned h)
{
	size[0] = w;
	size[1] = h;
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
	assert(gl_get_error());

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


shader::usage::usage (shader& ref, size_t verts, size_t inds) : shader_ref(ref)
{
	vertices = verts;
	indices = inds;
	texture_unit = 0;
}


shader::usage shader::usage::set_camera(const g::game::camera& cam)
{
	assert(gl_get_error());
	this->set_uniform("u_view").mat4(cam.view());
	this->set_uniform("u_proj").mat4(cam.projection());
	assert(gl_get_error());
	return *this;
}


shader::uniform_usage shader::usage::set_uniform(const std::string& name)
{
	GLint loc;
	auto it = shader_ref.uni_locs.find(name);
	if (it == shader_ref.uni_locs.end())
	{
		loc = glGetUniformLocation(shader_ref.program, name.c_str());

		if (loc < 0)
		{
			// TODO: handle the missing uniform better
			std::cerr << "uniform '" << name << "' doesn't exist\n";
			shader_ref.uni_locs[name] = loc;
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
	glUniformMatrix4fv(uni_loc, 1, false, m.ptr());

	return parent_usage;
}

shader::usage shader::uniform_usage::mat3 (const mat<3, 3>& m)
{
	glUniformMatrix3fv(uni_loc, 1, false, m.ptr());

	return parent_usage;
}

shader::usage shader::uniform_usage::vec3 (const vec<3>& v)
{
	glUniform3fv(uni_loc, 1, v.v);

	return parent_usage;
}

shader::usage shader::uniform_usage::flt (float f)
{
	glUniform1f(uni_loc, f);

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


GLuint shader_factory::compile_shader (GLenum type, const GLchar* src, GLsizei len)
{
	// Create the GL shader and attempt to compile it
	auto shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, &len);
	glCompileShader(shader);

	assert(gl_get_error());

	// Check the compilation status
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cerr << G_TERM_RED << "FAILED " << status << G_TERM_COLOR_OFF << std::endl;
		std::cerr << G_TERM_YELLOW << src << G_TERM_COLOR_OFF << std::endl;
	}
	assert(gl_get_error());

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
	assert(gl_get_error());

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

	assert(gl_get_error());
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
			write(1, log_str, log_length);
			free(log_str);
		}
		exit(-1);
	}
	else
	{
		std::cerr << "Linked program " << out.program << std::endl;
	}

	assert(gl_get_error());

	// Detach all
	for (auto shader : shaders)
	{
		glDetachShader(out.program, shader.second);
	}

	return out;
}
