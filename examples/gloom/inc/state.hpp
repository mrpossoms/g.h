#pragma once
#include <g.h>

#include <random>

#include "api/api_generated.h"

#define PLAYERS_MAX (256)

namespace gloom
{

struct State
{
	struct World
	{
			
	};


	struct Player
	{
		struct Session
		{
			unsigned id;
		};
	};

	std::default_random_engine rng;
	std::unordered_map<unsigned, State::Player> players;
	std::unordered_map<unsigned, State::Player::Session> sessions;
};

} // namespace gloom
