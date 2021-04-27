#include "g.gfx.h"

using namespace g::gfx;

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
	glGenTextures(1, &this->texture);
	assert(gl_get_error());
}

void texture::destroy()
{
	glDeleteTextures(1, &texture);
	release_bitmap();
}

void texture::set_pixels(size_t w, size_t h, char* data, GLenum format, GLenum storage, GLenum t)
{
	width = w;
	height = h;
	type = t;
	this->data = data;
	glTexImage2D(type, 0, format, width, height, 0, format, storage, data);
}

void texture::bind() const { glBindTexture(type, texture); }



texture_factory::texture_factory(int w, int h, GLenum type)
{
	data = nullptr;
	texture_type = type;
	width = w;
	height = h;
}

void texture_factory::abort(std::string message)
{
	std::cerr << message << std::endl;
	exit(-1);
}

texture_factory& texture_factory::from_png(const std::string& path)
{
	char header[8];    // 8 is the maximum size that can be checked
	png_structp png_ptr = {};
	png_infop info_ptr;
	png_bytep* row_pointers;
	png_byte png_color_type;

	std::cerr << "loading texture '" <<  path << "'... ";

	/* open file and test for it being a png */
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp)
	{
		fprintf(stderr, G_TERM_RED "[read_png_file] File %s could not be opened for reading" G_TERM_COLOR_OFF, path.c_str());
		return *this;
	}

	fread(header, 1, 8, fp);
	if (png_sig_cmp((png_bytep)header, 0, 8))
	{
		fprintf(stderr, G_TERM_RED "[read_png_file] File %s is not recognized as a PNG file" G_TERM_COLOR_OFF, path.c_str());
	}


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort(G_TERM_RED "[read_png_file] png_create_read_struct failed" G_TERM_COLOR_OFF);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort(G_TERM_RED "[read_png_file] png_create_info_struct failed" G_TERM_COLOR_OFF);

	if (setjmp(png_jmpbuf(png_ptr)))
		abort(G_TERM_RED "[read_png_file] Error during init_io" G_TERM_COLOR_OFF);

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	png_color_type = png_get_color_type(png_ptr, info_ptr);
	auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	auto channels = png_get_channels(png_ptr, info_ptr);
	auto color_depth = channels * (bit_depth >> 3);

	switch (bit_depth)
	{
		case 8:
			storage_type = GL_UNSIGNED_BYTE;
			break;
	}

	//number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		abort(G_TERM_RED "[read_png_file] Error during read_image" G_TERM_COLOR_OFF);
	}

	switch (png_color_type) {
		case PNG_COLOR_TYPE_RGBA:
			color_type = GL_RGBA;
			break;
		case PNG_COLOR_TYPE_PALETTE:
		case PNG_COLOR_TYPE_RGB:
			color_type = GL_RGB;
			break;
	}

	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	char* pixel_buf = (char*)calloc(color_depth * width * height, sizeof(char));
	int bytes_per_row = png_get_rowbytes(png_ptr,info_ptr);

	for (int y = 0; y < height; y++)
	{
		row_pointers[y] = (png_byte*) malloc(bytes_per_row); //
		assert(row_pointers[y]);
	}

	png_read_image(png_ptr, row_pointers);
	for (int y = 0; y < height; y++)
	{
		memcpy(pixel_buf + (y * bytes_per_row), row_pointers[y], bytes_per_row);
		free(row_pointers[y]);
	}
	free(row_pointers);
	fclose(fp);

	data = pixel_buf;

	std::cerr << G_TERM_GREEN "OK" G_TERM_COLOR_OFF << std::endl;

	return *this;
}

texture_factory& texture_factory::color()
{
	color_type = GL_RGBA;
	storage_type = GL_UNSIGNED_BYTE;
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
	wrap_s = wrap_t = GL_CLAMP_TO_EDGE;
	return *this;
}

texture_factory& texture_factory::repeating()
{
	wrap_s = wrap_t = GL_REPEAT;
	return *this;
}

texture texture_factory::create()
{
	texture out;

	out.create(texture_type);
	out.bind();

	assert(gl_get_error());
	out.set_pixels(width, height, data, color_type, storage_type);
	assert(gl_get_error());


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	assert(gl_get_error());
	// glGenerateMipmap(GL_TEXTURE_2D);
	assert(gl_get_error());

	return out;
}

framebuffer_factory::framebuffer_factory(int w, int h)
{
	width = w;
	height = h;
}

framebuffer_factory& framebuffer_factory::color()
{
	color_tex = texture_factory{ width, height }.color().clamped().smooth().create();
	return *this;
}

framebuffer_factory& framebuffer_factory::depth()
{
	depth_tex = texture_factory{ width, height }.depth().clamped().smooth().create();
	return *this;
}

framebuffer_factory& framebuffer_factory::shadow_map()
{
	return depth();
}

framebuffer framebuffer_factory::create()
{
	framebuffer fb;

	fb.width = width;
	fb.height = height;
	fb.color = color_tex;
	fb.depth = depth_tex;
	glGenFramebuffers(1, &fb.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	assert(gl_get_error());

	if (color_tex.texture != (GLuint)-1)
	{
		color_tex.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.texture, 0);
	}

	if (depth_tex.texture != (GLuint)-1)
	{
		depth_tex.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.texture, 0);
	}

	auto fb_stat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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
	this->set_uniform("u_view").mat4(cam.view());
	this->set_uniform("u_proj").mat4(cam.projection());
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

shader::usage shader::uniform_usage::vec3 (const vec<3>& v)
{
	glUniform3fv(uni_loc, 1, v.v);

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

	// Print the compilation log if there's anything in there
	GLint log_length;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		GLchar *log_str = (GLchar *)malloc(log_length);
		glGetShaderInfoLog(shader, log_length, &log_length, log_str);
		std::cerr << "Shader compile log: " << log_length << std::endl << log_str << std::endl;
		write(1, log_str, log_length);
		free(log_str);
	}

	assert(gl_get_error());

	// Check the status and exit on failure
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cerr << "Compiling failed: " << status << std::endl;
		glDeleteShader(shader);

		std::cerr << src << std::endl;
		exit(-2);
	}

	assert(gl_get_error());
	std::cerr << "OK" << std::endl;

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