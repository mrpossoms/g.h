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

			inline bool is_local() const { return id == 0; }
		};

		std::string name;

		vec<3> position;
		vec<3> velocity;
		vec<3> control;
		quat<> orientation;

		std::vector<ray>& rays() override
		{
			ray_list.clear();
			ray_list.push_back({position - vec<3>{0, 1, 0}, velocity});
			ray_list.push_back({position, velocity});

			return ray_list;
		}

		void pre_update (float dt, float time)
		{
			velocity += control * dt;
		}

		void update (float dt, float time)
		{
			position += velocity * dt;
		}
	};

	struct World
	{
		g::game::voxels<uint8_t> voxels;
		std::unique_ptr<g::game::voxel_collider<uint8_t>> collider;
		ogt_vox_palette palette;
	};

	float time;

	std::default_random_engine rng;
	g::asset::store assets;

	std::unordered_map<unsigned, State::Player> players;
	std::unordered_map<unsigned, State::Player::Session> sessions;

	World world;

	g::game::fps_camera camera;
};

} // namespace gloom
