#pragma once
#include <g.h>

#include <random>

#include "api/api_generated.h"

#define PLAYERS_MAX (256)

namespace gloom
{

struct State
{
	struct Player
	{
		struct Session
		{
			unsigned id;
			std::vector<vec<3, int>> requested_blocks;
		};

		std::string name;

		vec<3> position;
		vec<3> velocity;
		vec<3> control;
		quat<> orientation;

	};

	struct World
	{
		g::game::voxels<uint8_t> voxels;
	};

	std::default_random_engine rng;
	std::unordered_map<unsigned, State::Player> players;
	std::unordered_map<unsigned, State::Player::Session> sessions;
};

} // namespace gloom
