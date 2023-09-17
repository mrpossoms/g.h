#include "g.gfx.h"

#include <regex>


using namespace g::gfx;


struct opengl_texture : public g::gfx::texture
{
	GLuint id = 0;
	// uint8_t* data = nullptr;
	// texture::description desc = {};

	GLenum type() const
	{
		return (GLenum[]){ 
			GL_FALSE, 
			GL_UNSIGNED_BYTE, 
			GL_BYTE,
			GL_UNSIGNED_SHORT,
			GL_SHORT,
			GL_UNSIGNED_INT,
			GL_INT,
			GL_HALF_FLOAT,
			GL_FLOAT,
		}[desc.pixel_storage_type];
	}

	GLenum format() const
	{
		const static GLenum color_map[] = {
			GL_FALSE,
			GL_RED,
			GL_RG,
			GL_RGB,
			GL_RGBA
		};

		return color_map[desc.components()];
	}

	GLenum internal_format() const
	{
		if (desc.usage.depth)
		{
			const static GLenum color_map[] = {
				GL_FALSE,
				GL_FALSE,
				GL_FALSE,
				GL_DEPTH_COMPONENT16,
				GL_DEPTH_COMPONENT16,
				GL_DEPTH_COMPONENT32,
				GL_DEPTH_COMPONENT32,
				GL_FALSE,
				GL_DEPTH_COMPONENT32F,
			};

			return color_map[desc.pixel_storage_type];
		}
		else
		{
			const static GLenum color_map[][4] = {
				{ GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, },
				{ GL_R8UI, GL_RG8UI, GL_RGB8UI, GL_RGBA, },
				{ GL_R8I, GL_RG8I, GL_RGB8I, GL_RGBA8I, },
				{ GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI, },
				{ GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I, },
				{ GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI, },
				{ GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I, },
				{ GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F, },
				{ GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, },
			};

			return color_map[desc.pixel_storage_type][desc.components() - 1];			
		}
	}

	GLenum target() const
	{
		auto d = this->dimensions();
		
		if (d == 0 || d > 3) throw std::runtime_error("texture dimensions invalid");

		return (GLenum[3]){ GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D }[d - 1];
	}

	opengl_texture(const texture::description& desc)
	{
		glGenTextures(1, &id);
		data = nullptr;
		this->desc = desc;
	}

	~opengl_texture() override
	{
		this->release_bitmap();

		if (id != 0) 
		{
			glDeleteTextures(1, &id);
		}
	}

	opengl_texture(opengl_texture&& other)
	{
		id = other.id;
		other.id = 0;
	}

	opengl_texture& operator=(opengl_texture&& other)
	{
		id = other.id;
		other.id = 0;
		return *this;
	}

	opengl_texture(const opengl_texture& other) = delete;

	opengl_texture& operator=(const opengl_texture& other) = delete;

	inline bool is_initialized() const override
	{
		return id != 0;
	}

	void release_bitmap() override
	{
		if (data != nullptr)
		{
			free(data);
			data = nullptr;
		}
	}

	void set_pixels(
		const texture::description& desc,
		unsigned char* data) override
	{
		this->desc = desc;
		this->data = data;

		if (this->is_3D())
		{
			glTexImage3D(GL_TEXTURE_3D, 0, format(), desc.size[0], desc.size[1], desc.size[2], 0, internal_format(), type(), data);
		}
		else if (this->is_2D())
		{
			glTexImage2D(GL_TEXTURE_2D, 0, format(), desc.size[0], desc.size[1], 0, internal_format(), type(), data);
		}
	}

	void get_pixels(unsigned char** data_out, size_t& data_out_size) const override
	{
		data_out_size = desc.bytes();
		*data_out = new unsigned char[data_out_size];

		this->bind();	

		// TODO
	}

	void to_disk(const std::string& path) const override
	{

	}
	
	void bind() const override
	{
		glBindTexture(this->target(), id); 
	}

	void unbind() const override
	{
		glBindTexture(this->target(), 0); 
	}

	void set_filtering(texture::filter filter) override
	{		
		const static GLenum filter_map[] = {
			GL_NEAREST,
			GL_LINEAR,
		};

		auto type = this->target();
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, filter_map[filter]);
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, filter_map[filter]);
		glGenerateMipmap(type);
	}

	void set_wrapping(texture::wrap wrap) override
	{
		const static GLenum wrap_map[] = {
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_BORDER,
			GL_REPEAT,
			GL_MIRRORED_REPEAT,
		};

		auto type = this->target();
		glTexParameteri(type, GL_TEXTURE_WRAP_S, wrap_map[wrap]);
		glTexParameteri(type, GL_TEXTURE_WRAP_T, wrap_map[wrap]);
		glTexParameteri(type, GL_TEXTURE_WRAP_R, wrap_map[wrap]);
	}
};

struct opengl_framebuffer : public g::gfx::framebuffer
{
	GLuint id;
	opengl_texture* _color;
	opengl_texture* _depth;

	opengl_framebuffer(opengl_texture* color, opengl_texture* depth)
		: _color(color), _depth(depth)
	{
		glGenFramebuffers(1, &id);
		glBindFramebuffer(GL_FRAMEBUFFER, id);

		if (_color && _depth)
		{
			constexpr auto dims = sizeof(texture::description::size) / sizeof(texture::description::size[0]);
			for (unsigned i = 0; i < dims; i++)
			{
				if (_color->desc.size[i] != _depth->desc.size[i])
				{
					throw std::runtime_error("color and depth textures must have the same dimensions");
				}
			}
		}

		if (_color)
		{
			glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0, _color->id, 0);
			assert(gl_get_error());
		}

		if (_depth)
		{
			glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, _depth->id, 0);
			assert(gl_get_error());
		}

		auto fb_stat = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);

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
	}

	opengl_framebuffer(const opengl_framebuffer& other) = delete;

	opengl_framebuffer& operator=(const opengl_framebuffer& other) = delete;

	opengl_framebuffer(opengl_framebuffer&& other)
	{
		id = other.id;
		other.id = 0;
		_color = other._color;
		_depth = other._depth;
		other._color = other._depth = nullptr;
	}

	opengl_framebuffer& operator=(opengl_framebuffer&& other)
	{
		id = other.id;
		other.id = 0;
		_color = other._color;
		_depth = other._depth;
		other._color = other._depth = nullptr;
		return *this;
	}

	~opengl_framebuffer() override
	{
		if (id != 0) { glDeleteFramebuffers(1, &id); }
		if (_color) { delete _color; }
		if (_depth) { delete _depth; }
	}

	texture* color(texture* tex) override
	{
		if (tex)
		{
			if (!tex->desc.usage.depth)
			{
				_color = (opengl_texture*)tex;
				glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0, _color->id, 0);
			}
			else
			{
				throw std::runtime_error("color texture must have color usage");
			}
		}

		return _color;
	}

	texture* depth(texture* tex) override
	{
		if (tex)
		{
			if (tex->desc.usage.depth)
			{
				_depth = (opengl_texture*)tex;
				glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, _depth->id, 0);
			}
			else
			{
				throw std::runtime_error("depth texture must have depth usage");
			}
		}

		return _depth;
	}

	unsigned* size() override
	{
		if (_color)
		{
			return _color->desc.size;
		}
		else if (_depth)
		{
			return _depth->desc.size;
		}
		else
		{
			throw std::runtime_error("framebuffer::size() called on framebuffer with no associated textures");
		}
	}

	float aspect() const override
	{
		if (_color)
		{
			return _color->desc.size[0] / (float)_color->desc.size[1];
		}
		else if (_depth)
		{
			return _depth->desc.size[0] / (float)_depth->desc.size[1];
		}
		else
		{
			throw std::runtime_error("framebuffer::aspect() called on framebuffer with no associated textures");
		}
	}	

	void bind_as_target() override
	{
		auto s = this->size();
		bind_as_target({ 0, 0 }, { s[0], s[1] });
	}

	void bind_as_target(const vec<2, unsigned>& upper_left, const vec<2, unsigned>& lower_right) override
	{
		auto w = lower_right[0] - upper_left[0], h = lower_right[1] - upper_left[1];
		glViewport(upper_left[0], upper_left[1], w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, id);
		assert(gl_get_error());
	}

	void unbind_as_target() override
	{
		if (_color)
		{
			_color->bind();
			glGenerateMipmap(_color->target());
		}

		glViewport(0, 0, g::gfx::width(), g::gfx::height());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};

g::gfx::api::opengl::opengl()
{
	// NOP
}

g::gfx::api::opengl::~opengl()
{
	glfwTerminate();
}

static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;
}

void g::gfx::api::opengl::initialize(const api::options& gfx, const char* name)
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) { throw std::runtime_error("glfwInit() failed"); }

	// api specific hints pre context creation
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gfx.api_version.major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gfx.api_version.minor);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (gfx.fullscreen)
	{
		auto monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		// glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		// glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		// glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		// glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);					
		win = glfwCreateWindow(mode->width, mode->height, name ? name : "", monitor, NULL);
	}
	else
	{
		win = glfwCreateWindow(gfx.width, gfx.height, name ? name : "", NULL, NULL);
	}

	if (!win)
	{
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow() returned NULL");
	}

	glfwMakeContextCurrent(win);

	// TODO: this is a vile hack. I need to find a good way to decouple graphics from input
	g::gfx::GLFW_WIN = win;

	GLuint vao;
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context" << std::endl;
		throw std::runtime_error("glad runtime error");
	}

	auto glsl_ver_str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cerr << "GL renderer: " << glGetString(GL_VERSION) << std::endl;
	std::cerr << "GLSL version: " << glsl_ver_str << std::endl;

	// set the correct glsl shader header based on the version we found
	std::cmatch m;
	std::regex re("[0-9]+[.][0-9]+");
	if (std::regex_search(glsl_ver_str, m, re))
	{
		std::string version = m[0];
		version.erase(version.find("."), 1);

		g::gfx::shader_factory::shader_path = std::string("glsl/") + version + std::string("/");
		g::gfx::shader_factory::shader_header = std::string("#version ") + version + std::string("\n");
	}
	else
	{
		std::cerr << "Couldn't identify glsl version" << std::endl;
	}

	// Hack, webgl related IIRC
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Good defaults, enabling depth testing, transparency and backface culling
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void g::gfx::api::opengl::pre_draw()
{
	glfwGetFramebufferSize(win, &frame.width, &frame.height);
}

void g::gfx::api::opengl::post_draw()
{
	glfwSwapBuffers(g::gfx::GLFW_WIN);
	glfwPollEvents();
	glViewport(0, 0, frame.width, frame.height);
}

size_t g::gfx::api::opengl::width()
{
	return frame.width;
}

size_t g::gfx::api::opengl::height()
{
	return frame.height;
}

float g::gfx::api::opengl::aspect()
{
	return frame.width / (float)frame.height;
}

texture* g::gfx::api::opengl::make_texture(const texture::description& desc)
{
	return new opengl_texture(desc);
}

framebuffer* g::gfx::api::opengl::make_framebuffer(texture* color, texture* depth)
{
	return new opengl_framebuffer((opengl_texture*)color, (opengl_texture*)depth);
}