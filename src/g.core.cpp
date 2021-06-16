#include "g.h"
#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

GLFWwindow* g::gfx::GLFW_WIN;

void g::core::tick()
{
	auto t_0 = std::chrono::system_clock::now();
	std::chrono::duration<float> dt = t_0 - t_1;

	update(dt.count());
	t_1 = t_0;

	if (g::gfx::GLFW_WIN)
	{
		glfwSwapBuffers(g::gfx::GLFW_WIN);
		glfwPollEvents();
		running = !glfwWindowShouldClose(g::gfx::GLFW_WIN);
	}
}

static void error_callback(int error, const char* description)
{
    std::cerr << description << std::endl;
}

static void EMSCRIPTEN_MAIN_LOOP(void* arg)
{
	static_cast<g::core*>(arg)->tick();
}

void g::core::start(const core::opts& opts)
{
	if (opts.gfx.display)
	{
		glfwSetErrorCallback(error_callback);

		if (!glfwInit()) { throw std::runtime_error("glfwInit() failed"); }

		// api specific hints pre context creation
		switch (opts.gfx.api)
		{
			case g::core::opts::render_api::OPEN_GL:
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opts.gfx.api_version.major);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opts.gfx.api_version.minor);
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				break;
		}

		g::gfx::GLFW_WIN = glfwCreateWindow(opts.gfx.width, opts.gfx.height, opts.name ? opts.name : "", NULL, NULL);

		if (!g::gfx::GLFW_WIN)
		{
			glfwTerminate();
			throw std::runtime_error("glfwCreateWindow() returned NULL");
		}

		glfwMakeContextCurrent(g::gfx::GLFW_WIN);

		std::cerr << "GL renderer: " << glGetString(GL_VERSION) << std::endl;

	}

	// rendering api standard config post context creation
	switch (opts.gfx.api)
	{
		case g::core::opts::render_api::OPEN_GL:
		{
			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glEnable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
			break;
	}

	if (!initialize()) { throw std::runtime_error("User initialize() call failed"); }



#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(EMSCRIPTEN_MAIN_LOOP, this, 144, 1);
#else
	while (running)
	{ // TODO: refactor this such that a 'main loop' function can be implemented and called by emscripten_set_main_loop
		tick();
	}
#endif

}


void g::utils::base64_encode(void *dst, const void *src, size_t len) // thread-safe, re-entrant
{
	static const unsigned char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	assert(dst != src);
	unsigned int *d = (unsigned int *)dst;
	const unsigned char *s = (const unsigned char*)src;
	const unsigned char *end = s + len;

	while(s < end)
	{
		uint32_t e = *s++ << 16;
		if (s < end) e |= *s++ << 8;
		if (s < end) e |= *s++;
		*d++ = b64[e >> 18] | (b64[(e >> 12) & 0x3F] << 8) | (b64[(e >> 6) & 0x3F] << 16) | (b64[e & 0x3F] << 24);
	}
	for (size_t i = 0; i < (3 - (len % 3)) % 3; i++) ((char *)d)[-1-i] = '=';
}
