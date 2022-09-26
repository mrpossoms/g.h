#include "g.h"
#include <mutex>
// #define OGT_VOX_IMPLEMENTATION
// #define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

struct voxel_world : public g::core
{
	g::asset::store assets;

	g::gfx::mesh<g::gfx::vertex::pos_norm_color> temple;
	g::gfx::mesh<g::gfx::vertex::pos_norm_color> light_mesh;
	g::gfx::framebuffer shadow_map, render_target;

	g::game::voxels_paletted light_vox;
	g::game::camera_perspective cam;
	g::game::camera_orthographic light;
	float t = 0;

	virtual bool initialize()
	{
		{ // graphics init
			shadow_map = g::gfx::framebuffer_factory{1024, 1024}.color().depth().create();
			render_target = g::gfx::framebuffer_factory{(unsigned)g::gfx::width(), (unsigned)g::gfx::height()}.color().depth().create();
		}


		ogt_vox_palette pal = {
			{
				{ 255, 255, 255, 255 },
				{ 255, 255, 255, 255 },
			}
		};
		uint8_t color = 1;
		light_vox = g::game::voxels_paletted(pal, &color, 1, 1, 1);
		light_mesh = g::gfx::mesh_factory::from_voxels<g::gfx::vertex::pos_norm_color>(light_vox,
		[](ogt_mesh_vertex* v) -> g::gfx::vertex::pos_norm_color {
			return {
				{ v->pos.x, v->pos.y, v->pos.z },
				{ v->normal.x, v->normal.y, v->normal.z },
				{ v->color.r, v->color.g, v->color.b, v->color.a },
			};
		});

		temple = g::gfx::mesh_factory::from_voxels<g::gfx::vertex::pos_norm_color>((g::game::voxels<uint8_t>)assets.vox("farm.vox").flatten(), assets.vox("farm.vox").palette,
		[](ogt_mesh_vertex* v) -> g::gfx::vertex::pos_norm_color {
			return {
				{ v->pos.x, v->pos.y, v->pos.z },
				{ v->normal.x, v->normal.y, v->normal.z },
				{ v->color.r, v->color.g, v->color.b, v->color.a },
			};
		});

		//assets.vox("temple.vox").center_of_mass(true);

		cam.position = assets.vox("farm.vox").center().cast<float>() + vec<3>{2, 0, 0};
		light.width = 42;
		light.height = 55;

		glDisable(GL_CULL_FACE);

		return true;
	}

	virtual void update(float dt)
	{

		const auto speed = 4.0f;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt * speed;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt * speed;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * -dt * speed;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * dt * speed;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) return;

		// cam.orientation = quat::from_axis_angle({0, 0, 1}, 0);

		auto model = mat4::I();
		light.position = vec<3>{cos(t * 0.1f) * 60, sin(t * 0.1f) * 60, 60};
		light.look_at(vec<3>{0, 0, 0}, vec<3>{0, 0, 1});

		{ // draw shadow map
			g::gfx::framebuffer::scoped_draw sd(shadow_map);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			temple.using_shader(assets.shader("depth_only.vs+depth_only.fs"))
				.set_camera(light)
				["u_model"].mat4(model)
				.draw<GL_TRIANGLES>();

		}
		

		{
			// g::gfx::framebuffer::scoped_draw sd(render_target);

			glClearColor(1, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cam.aspect_ratio(g::gfx::aspect());

			auto light_model = mat4::translation(light.position + light_vox.center_of_bounds() * -1);
			light_mesh.using_shader(assets.shader("basic_color.vs+basic_color.fs"))
			.set_camera(cam)
			["u_model"].mat4(light_model)
			.draw<GL_TRIANGLES>();

			temple.using_shader(assets.shader("basic_color.vs+basic_color.fs"))
			.set_camera(cam)
			["u_model"].mat4(model)
			["u_light_view"].mat4(light.view())
			["u_light_proj"].mat4(light.projection())
			["u_light_diffuse"].vec3({1, 1, 1})
			["u_light_ambient"].vec3({13.5f/255.f, 20.6f/255.f, 23.5f/255.f})
			.draw<GL_TRIANGLES>();
		}

		{
			g::gfx::effect::shadow_0<g::gfx::vertex::pos_norm_color>(temple, shadow_map, light, cam, [&](g::gfx::shader::usage& chain) {
				chain["u_model"].mat4(model);
			});
		}

		t += dt;
	}
};


int main (int argc, const char* argv[])
{
	voxel_world game;

	game.start({ 
        .name = argv[0],
        .gfx = {
            .display = true,
            .width = 1024,
            .height = 768 
        }
    });

	return 0;
}
