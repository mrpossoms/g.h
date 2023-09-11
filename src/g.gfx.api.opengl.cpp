#include "g.gfx.h"

#include <regex>


using namespace g::gfx;


struct opengl_texture : public g::gfx::texture
{
	GLuint id = 0;

	GLenum texture_type()
	{
		auto d = this->dimensions();
		
		if (d == 0 || d > 3) throw std::runtime_error("texture dimensions invalid");

		return { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D }[d - 1];
	}

	opengl_texture(size_t w, size_t h, size_t d, g::gfx::texture::format f, g::gfx::texture::type t)
		: size(w, h, d), format(f), type(t)
	{
		glGenTextures(1, &id);
	}

	~opengl_texture()
	{
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
		size_t w, size_t h, size_t d, 
		unsigned char* data, 
		texture::pixel_type storage_type=pixel_type::uint8) override
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

	void get_pixels(unsigned char** data_out, size_t& data_out_size) const override
	{
		data_out_size = size[0] * size[1] * size[2];
		*data_out = new unsigned char[data_out_size];

		GLenum storage_map[] = {
			GL_FALSE,
			GL_UNSIGNED_BYTE,
			GL_UNSIGNED_BYTE,
			GL_FALSE,
			GL_UNSIGNED_BYTE
		};

		GLenum color_map[] = {
			GL_FALSE,
			GL_RED,
			GL_RG,
			GL_RGB,
			GL_RGBA
		};

		this->bind();	
	}

	void to_disk(const std::string& path) const override
	{

	}

	size_t bytes() const override
	{
		auto pixels = size[0] * size[1] * size[2];
		return pixels * bytes_per_component * component_count;
	}
	
	void bind() const override
	{
		glBindTexture(this->texture_type(), id); 
	}

	void unbind() const override
	{
		glBindTexture(this->texture_type(), 0); 
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