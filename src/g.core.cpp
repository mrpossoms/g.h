#include "g.h"
#include <chrono>
#include <filesystem>
#include <limits.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

GLFWwindow* g::gfx::GLFW_WIN = nullptr;


static std::string executable_path()
{
#if defined(_WIN32)
	WCHAR path_buf[MAX_PATH];
	GetModuleFileNameW(NULL, path_buf, MAX_PATH);
	return std::filesystem::path(path_buf).remove_filename().string();
#elif defined(__linux__)
	char path_buf[PATH_MAX];
	if (readlink("/proc/self/exe", path_buf, sizeof(path_buf) - 1) <= 0)
	{
		std::cerr << G_TERM_RED << "reading exe path link failed: " << std::string(strerror(errno)) << G_TERM_COLOR_OFF << std::endl;
		return "";
	}
	return std::filesystem::path(path_buf).remove_filename().string();
#endif
}

void g::core::tick()
{
	auto t_0 = std::chrono::system_clock::now();
	std::chrono::duration<float> dt = t_0 - t_1;

	if (g::gfx::api::instance != nullptr)
	{
		g::gfx::api::instance->pre_draw();
	}

	update(dt.count());
	t_1 = t_0;

	if (g::gfx::api::instance != nullptr)
	{
		g::gfx::api::instance->post_draw();
		// TODO: migrate to api::interface
		running &= !glfwWindowShouldClose(g::gfx::GLFW_WIN);
	}
}

#ifdef __EMSCRIPTEN__
static void EMSCRIPTEN_MAIN_LOOP(void* arg)
{
	static_cast<g::core*>(arg)->tick();
}
#endif

void g::core::start(const core::opts& opts)
{
	auto exe_path = executable_path();

	if (exe_path.length() == 0)
	{ 
		std::cerr << G_TERM_RED << "exe path zero length" <<  G_TERM_COLOR_OFF << std::endl;
		return;
	}
	std::filesystem::current_path(exe_path);

	if (opts.gfx.display)
	{
		// TODO: construct the appropriate graphics api instance based on user request
		g::gfx::api::instance = std::make_unique<g::gfx::api::opengl>();
		g::gfx::api::instance->initialize(opts.gfx, opts.name);
	}

	if (opts.snd.enabled)
	{
		g::snd::initialize();
	}

	if (!initialize()) { throw std::runtime_error("User initialize() call failed"); }



#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(EMSCRIPTEN_MAIN_LOOP, this, 0, 1);
#else
	while (running)
	{
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
