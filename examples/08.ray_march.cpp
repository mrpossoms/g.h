#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

struct volumetric : public g::core
{
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::game::camera_perspective cam;
	g::asset::store assets;

	virtual bool initialize()
	{
		plane = g::gfx::mesh_factory::plane();

		return true;
	}

	virtual void update(float dt)
	{

		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) return;

	
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		plane.using_shader(assets.shader("basic_post.vs+raymarch.fs")).draw<GL_TRIANGLE_FAN>();
	}
};


int main (int argc, const char* argv[])
{
	volumetric game;

	game.start({ "volume", true, 1024, 768 });

	return 0;
}
