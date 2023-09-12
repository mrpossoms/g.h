#include "g.gfx.h"

#include <regex>


using namespace g::gfx;


struct opengl_texture : public g::gfx::texture
{
	GLuint id = 0;

	GLenum type()
	{
		auto d = this->dimensions();
		
		if (d == 0 || d > 3) throw std::runtime_error("texture dimensions invalid");

		return { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D }[d - 1];
	}

	GLenum format()
	{
		const static GLenum color_map[] = {
			GL_FALSE,
			GL_RED,
			GL_RG,
			GL_RGB,
			GL_RGBA
		};

		return color_map[desc.pixel_type];
	}

	GLenum internal_format()
	{
		if (desc.usage.depth)
		{
			const static GLenum color_map[] = {
				GL_FALSE,
				GL_DEPTH_COMPONENT16,
				GL_DEPTH_COMPONENT24,
				GL_DEPTH_COMPONENT32,
				GL_DEPTH_COMPONENT32F,
			};

			return color_map[desc.pixel_type];
		}
		else
		{
			const static GLenum color_map[][4] = {
				{ GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, },
				{ GL_R8UI, GL_RG8UI, GL_RGB8UI, GL_RBGA8UI, },
				{ GL_R8I, GL_RG8I, GL_RGB8I, GL_RBGA8I, },
				{ GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RBGA16UI, },
				{ GL_R16I, GL_RG16I, GL_RGB16I, GL_RBGA16I, },
				{ GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RBGA32UI, },
				{ GL_R32I, GL_RG32I, GL_RGB32I, GL_RBGA32I, },
				{ GL_R16F, GL_RG16F, GL_RGB16F, GL_RBGA16F, },
				{ GL_R32F, GL_RG32F, GL_RGB32F, GL_RBGA32F, },
			};

			return color_map[desc.pixel_type];			
		}
	}

	GLenum target()
	{
		auto d = this->dimensions();
		
		if (d == 0 || d > 3) throw std::runtime_error("texture dimensions invalid");

		return { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D }[d - 1];
	}

	opengl_texture(const texture::description& desc)
		: data(nullptr), desc(desc)
	{
		glGenTextures(1, &id);
	}

	~opengl_texture()
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

		if (h > 1 && d > 1)
		{
			glTexImage3D(GL_TEXTURE_3D, 0, internal_format(), size[0], size[1], size[2], 0, format(), type(), data);
		}
		else if (h >= 1)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format(), size[0], size[1], 0, format(), type(), data);
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
	glfwGetFramebufferSize(win, &framebuffer.width, &framebuffer.height);
}

void g::gfx::api::opengl::post_draw()
{
	glfwSwapBuffers(g::gfx::GLFW_WIN);
	glfwPollEvents();
	glViewport(0, 0, framebuffer.width, framebuffer.height);
}

size_t g::gfx::api::opengl::width()
{
	return framebuffer.width;
}

size_t g::gfx::api::opengl::height()
{
	return framebuffer.height;
}

float g::gfx::api::opengl::aspect()
{
	return framebuffer.width / (float)framebuffer.height;
}

texture* g::gfx::api::opengl::make_texture(const texture::description& desc)
{
	return new opengl_texture(w, h, d, f, t);
}
