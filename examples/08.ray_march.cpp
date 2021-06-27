#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

struct volumetric : public g::core
{
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::game::camera_perspective cam;
	g::asset::store assets;
	float t;

	GLuint cube;

	virtual bool initialize()
	{
		plane = g::gfx::mesh_factory::plane();

		cam.position = { 0, 1, 0 };

		glGenTextures(1, &cube);
		glBindTexture(GL_TEXTURE_3D, cube);

		float data[3][3][3];

		for (unsigned i = 0; i < 3; i++)
		for (unsigned j = 0; j < 3; j++)
		for (unsigned k = 0; k < 3; k++)
		{
			if (0 == i && 0 == j && 0 == k) { data[i][j][k] = -1; }
			else { data[i][j][k] = 1; }
		}			

		glTexImage3D(
			GL_TEXTURE_3D,
			0,
			GL_RED,
			3, 3, 3,
			0,
			GL_RED,
			GL_FLOAT,
			data
		);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

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
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.position += cam.up() * dt;

	
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_CULL_FACE);

		xmath::mat<4, 1> v = {
			{0}, {0}, {1000}, {1}
		};

		// auto p = cam.projection() * v;
		// p /= p[0][3];

		// auto u = cam.projection().invert() * p;
		// u /= u[0][3];

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, cube);
		plane.using_shader(assets.shader("raymarch.vs+raymarch.fs"))
		     .set_camera(cam)
		     ["u_cube"].int1(0)
		     .draw<GL_TRIANGLE_FAN>();

		t += dt;
	}
};


int main (int argc, const char* argv[])
{
	volumetric game;

	game.start({ "volume", true, 1024, 768 });

	return 0;
}
