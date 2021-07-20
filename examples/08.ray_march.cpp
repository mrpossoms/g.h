#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

struct volumetric : public g::core
{
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::game::camera_perspective cam;
	g::asset::store assets;
	float t, sub_step = 0.1f;

	GLuint cube;

	virtual bool initialize()
	{
		plane = g::gfx::mesh_factory::plane();

		cam.position = { 0, 1, 0 };

		glGenTextures(1, &cube);
		glBindTexture(GL_TEXTURE_3D, cube);

		const unsigned w=32, h=32, d=32;
		float data[w][h][d];

		for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++)
		for (int k = 0; k < d; k++)
		{
			// if (i == w / 2 && j == h / 2 && k == d / 2) { data[i][j][k] = -1.f; }
			if ((i + j + k) % 2 == 0) { data[i][j][k] = -1.f; }
			else { data[i][j][k] = 1.f; }
			// if (i == 15 && j == 15 && k == 15) { data[i][j][k] = -1; }
			// else
			// {
			// 	auto di = i - 15, dj = j - 15, dk = k - 15;
			// 	data[i][j][k] = floor(sqrt(di * di + dj * dj + dk * dk));
			// }
		}

		glTexImage3D(
			GL_TEXTURE_3D,
			0,
			GL_RED,
			w, h, d,
			0,
			GL_RED,
			GL_FLOAT,
			data
		);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

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
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(-dt);
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

		int show = 0;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Z)) { show = 1; }
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS) sub_step += 0.1f * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS) sub_step -= 0.1f * dt;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, cube);
		plane.using_shader(assets.shader("raymarch.vs+raymarch.fs"))
		     .set_camera(cam)
		     ["u_view_pos"].vec3(cam.position)
		     ["u_cube"].int1(0)
		     ["u_show"].int1(show)
		     ["u_sub_step"].flt(sub_step)
		     .draw<GL_TRIANGLE_FAN>();

		t += dt;
	}
};


int main (int argc, const char* argv[])
{
	volumetric game;

	game.start({ "volume", true, 640, 480 });

	return 0;
}
