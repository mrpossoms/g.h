#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

const unsigned w=32, h=32, d=32;
float data[w][h][d];

struct volumetric : public g::core
{
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::framebuffer render_buffer;
	g::game::camera_perspective cam;
	g::asset::store assets;

	float t, sub_step = -0.5f;

	GLuint cube;

	virtual bool initialize()
	{
		plane = g::gfx::mesh_factory::plane();

		render_buffer = g::gfx::framebuffer_factory{1024 >> 0, 768 >> 0}.color().create();

		cam.position = { 0, 1, 0 };

		for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++)
		for (int k = 0; k < d; k++)
		{
			// if (i == 0 || i == w-1) { data[i][j][k] = 1.f; continue; }
			// if (j == 0 || j == h-1) { data[i][j][k] = 1.f; continue; }
			// if (k == 0 || k == d-1) { data[i][j][k] = 1.f; continue; }
			// if (i == w / 2 && j == h / 2 && k == d / 2) { data[i][j][k] = -1.f; }
			const auto b = 0;
			const int hw = w >> 1;
			const int hd = d >> 1;

			auto ikd = sqrt(pow(i - hw, 2.0) + pow(k - hd, 2.0));

			if (
				(i >= b && i < w-b &&
				 j >= b && j < h-b &&
				 k >= b && k < d-b)
				&& ikd < w >> 1
				// && (i+j+k) % 2
				// && i == j && j == k
			)
			// if ((i + j + k) % 2)
			{
				data[i][j][k] = 0.f;
			}
			else 
			{ 
				data[i][j][k] = 1.f; 
			}

			// if (i == 7 && j == 7 && k == 7) { data[i][j][k] = 0; }
			// else
			// {
			// 	auto di = i - 8, dj = j - 8, dk = k - 8;
			// 	data[i][j][k] = floor(sqrt(di * di + dj * dj + dk * dk));
			// }
		}

		auto vox = assets.vox("probe.vox");

		std::cout << "vox: " << vox.width << ", " << vox.height << ", " << vox.depth << std::endl;

		glGenTextures(1, &cube);
		glBindTexture(GL_TEXTURE_3D, cube);

		glTexImage3D(
			GL_TEXTURE_3D,
			0,
			GL_RED,
			vox.width, vox.height, vox.depth,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			vox.v
		);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		cam.position = {0, 10, 0};

		return true;
	}

	void controls(float dt)
	{
		const auto speed = 4.f;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * speed * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * speed * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * speed * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * speed * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.position += cam.up() * speed * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.position -= cam.up() * speed * dt;
	}

	virtual void update(float dt)
	{
		this->controls(dt);

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
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS) { 
			sub_step += dt;
			std::cout << "radius bias: " << sub_step << std::endl;
		}	
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS) {
			sub_step -= dt;
			std::cout << "radius bias: " << sub_step << std::endl;
		}

		auto ld = xmath::vec<3>{ 0.6f, -1, 1};

		render_buffer.bind_as_target();
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, cube);
		plane.using_shader(assets.shader("raymarch.vs+raymarch.fs"))
		     .set_camera(cam)
		     ["u_view_pos"].vec3(cam.position)
		     ["u_cube"].int1(0)
		     ["u_show"].int1(show)
		     ["u_sub_step"].flt(sub_step)
		     ["u_light_pos"].vec3(cam.position + cam.up())
		     // ["u_light_pos"].vec3({20 * cos(t), 100, 20 * sin(t)})
		     .draw<GL_TRIANGLE_FAN>();

		render_buffer.unbind_as_target();

		plane.using_shader(assets.shader("basic_post.vs+basic_texture.fs"))
			["u_texture"].texture(render_buffer.color)
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
