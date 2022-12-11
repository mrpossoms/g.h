#pragma once
#include <g.h>

#include "state.hpp"

namespace gloom
{
namespace gameplay
{

gloom::Result load_level(gloom::State& state, const std::string& name)
{
	try
	{
		state.world.voxels = state.assets.vox(name).flatten();
	}
	catch (std::runtime_error e)
	{
		return false;
	}

	return true;
}

unsigned new_player_id(gloom::State& state)
{
	std::uniform_int_distribution<unsigned> id_dist(0,0xFFFFFFFF);
	unsigned id = 0;

	while (state.sessions.contains(id))
	{
		id = id_dist(state.rng);
	}

	return id;
}

} // namespace gameplay
} // namespace gameplay