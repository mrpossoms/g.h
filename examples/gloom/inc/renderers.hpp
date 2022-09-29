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

}