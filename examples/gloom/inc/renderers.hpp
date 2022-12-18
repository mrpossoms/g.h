#pragma once
#include <g.h>

#include "state.hpp"

namespace gloom
{

struct Renderer
{
	virtual void update(float dt) {}

	virtual void draw(float dt, gloom::State& state) = 0;
};


struct TitleScreenRenderer : public Renderer
{
		void draw(float dt, gloom::State& state) override
		{
			glClearColor(1, 0, 0, 1);
		}
};

struct GameRenderer : public Renderer
{
	g::gfx::mesh<g::gfx::vertex::pos_norm_color> level_mesh;

	void draw(float dt, gloom::State& state) override
	{
		if (!level_mesh)
		{
			level_mesh = g::gfx::mesh_factory::from_voxels<g::gfx::vertex::pos_norm_color>((g::game::voxels<uint8_t>)
				state.world.voxels,
				state.world.palette,
				[](ogt_mesh_vertex* v) -> g::gfx::vertex::pos_norm_color {
					return {
						{ v->pos.x, v->pos.y, v->pos.z },
						{ v->normal.x, v->normal.y, v->normal.z },
						{ v->color.r, v->color.g, v->color.b, v->color.a },
					};
				}
			);
		}

		glClearColor(1, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// TODO: temporary
		auto model = mat<4,4>::translation(state.assets.vox("temple.vox").center().cast<float>() * -1);

		level_mesh.using_shader(state.assets.shader("basic_color.vs+basic_color.fs"))
			.set_camera(state.camera)
			["u_model"].mat4(model)
			.draw<GL_TRIANGLES>();
	}
};

}