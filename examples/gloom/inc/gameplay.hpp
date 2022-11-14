#pragma once
#include <g.h>

#include "state.hpp"

namespace gloom
{
namespace gameplay
{

unsigned new_player_id(const gloom::State& state)
{
	std::uniform_int_distribution<unsigned> id_dist(0,0xFFFFFFFF);
	unsigned id = 0;

	while (state.contains(id))
	{
		id = id_dist(state.rng);
	}

	return id;
}

} // namespace gameplay
} // namespace gameplay