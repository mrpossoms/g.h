#include "g.gfx.h"

#include <regex>


using namespace g::gfx;

g::gfx::api::opengl::opengl()
{
	// NOP
}

g::gfx::api::opengl::~opengl()
{
	// TODO: teardown
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
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	auto glsl_ver_str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cerr << "GL renderer: " << glGetString(GL_VERSION) << std::endl;
	std::cerr << "GLSL version: " << glsl_ver_str << std::endl;

	// set the correct glsl shader header based on the version we found
	std::cmatch m;
	std::regex re("[0-9]+[.][0-9]+");
	if(std::regex_search (glsl_ver_str, m, re))
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

	GLuint vao;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "glew Error: " << glewGetErrorString(err) << std::endl;
		/* Problem: glewInit failed, something is seriously wrong. */
		throw std::runtime_error("glew runtime error");
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