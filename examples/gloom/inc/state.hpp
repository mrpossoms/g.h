#pragma once
#include <g.h>

#include <random>

#include "api/api_generated.h"

#define PLAYERS_MAX (256)

namespace gloom
{

using Result = bool;

struct State
{
	struct Player final : public updateable, ray_collider
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

		void pre_update (float dt, float time)
		{

		}

		void update (float dt, float time)
		{

		}
	};

	struct World
	{
		g::game::voxels<uint8_t> voxels;
		ogt_vox_palette palette;
	};

	std::default_random_engine rng;
	g::asset::store assets;

	std::unordered_map<unsigned, State::Player> players;
	std::unordered_map<unsigned, State::Player::Session> sessions;

	World world;

	g::game::fps_camera camera;
};

} // namespace gloom
