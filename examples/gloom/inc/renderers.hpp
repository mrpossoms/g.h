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
			glClearColor(255, 0, 0);
		}
};

struct GameRenderer : public Renderer
{
	g::gfx::mesh<g::gfx::vertex::pos_norm_color> level_mesh;

	void draw(float dt, gloom::State& state) override
	{
		glClearColor(255, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		level_mesh.using_shader(assets.shader("depth_only.vs+depth_only.fs"))
			.set_camera(light)
			["u_model"].mat4(model)
			.draw<GL_TRIANGLES>();
	}
};

}