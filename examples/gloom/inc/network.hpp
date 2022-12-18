#pragma once
#include <memory>

#include <g.h>

#include "state.hpp"
#include "gameplay.hpp"
#include "api/api_generated.h"
#include "flatbuffers/flatbuffers.h"

namespace gloom
{
namespace network
{



static std::shared_ptr<g::net::client> make_client(const std::string& hostname, gloom::State& state)
{
	auto client = std::make_shared<g::net::client>();

	client->on_connection = [&](socket_t sock) -> int
	{
		flatbuffers::FlatBufferBuilder builder(0xFFFF);

		auto config_command = gloom::api::commands::CreateConfigureCommand(builder, builder.CreateString("foob"));
		auto player_command = gloom::api::commands::CreatePlayer(builder, config_command);
		builder.Finish(player_command);

		write(sock, builder.GetBufferPointer(), builder.GetSize());

		return 0;
	};

	client->on_packet = [&](socket_t sock) -> int
	{

		return 0;
	};

	client->on_disconnection = [&](socket_t sock) {

	};

	client->connect(hostname, 1337);

	return client;
}

struct Host final : public g::net::host<State::Player::Session>
{
	Host() = default;

	void send_states(gloom::State& state)
	{
		// Player info gets sent to all players every time
		gloom::api::state::Player players[state.players.size()];
		std::vector<const gloom::api::state::Player*> players_vec;
		std::vector<flatbuffers::Offset<gloom::api::state::PlayerConfig>> player_configs_vec;

		auto world_size = gloom::Vec3ui(state.world.voxels.size.cast<uint32_t>().v);
		auto world_info = gloom::api::state::world::Info(
			state.world.voxels.hash(),
			world_size
		);

		// build up list of player states
		unsigned i = 0;
		for (auto& sock_sess_pair : state.players)
		{
			auto& id = sock_sess_pair.first;
			auto& player = sock_sess_pair.second;

			auto pos = gloom::Vec3f(player.position.v);
			auto vel = gloom::Vec3f(player.velocity.v);
			auto ori = gloom::Quat(player.orientation.v);
			players[i] = gloom::api::state::Player(id, pos, vel, ori);

			players_vec.push_back({&players[i]});
			// TODO: player_configs_vec
			i++;
		}

		for (auto& sock_sess_pair : state.sessions)
		{
			auto& sock = sock_sess_pair.first;
			auto& sess = sock_sess_pair.second;
			flatbuffers::FlatBufferBuilder builder(0xFFFF);

			gloom::api::state::gameBuilder gs_builder(builder);
			gs_builder.add_world(&world_info);
			gs_builder.add_players(builder.CreateVector(players_vec));
			gs_builder.add_player_configs(builder.CreateVector(player_configs_vec));

			// TODO: player specific updates to game state here
			gs_builder.add_my_id(sock);

			// We will not send more than 15 chunk deltas per step
			for (unsigned i = 0; i < 15 && sess.requested_blocks.size() > 0; i++)
			{
				auto& corner = sess.requested_blocks.back();
				uint8_t block[4096];

				// TODO: there should be a copy_block function added to the voxels class
				for (unsigned x_i = 0; x_i < 16; x_i++)
				{
					auto slice = state.world.voxels[x_i + corner[0]];
					constexpr auto x_stride = 16 * 16;

					for (unsigned y_i = 0; y_i < 16; y_i++)
					{
						constexpr auto y_stride = 16;
						memcpy(
							block + (x_i * x_stride) + (y_i * y_stride),
							slice[y_i + corner[1]], 
							sizeof(uint8_t) * y_stride
						);
					}
				}

				sess.requested_blocks.pop_back();

				gloom::api::state::world::Delta chunk_d;


			}
			
			auto game_state = gs_builder.Finish();
			write(sock, &game_state, sizeof(game_state)); // TODO: this use of game_state is not correct, need to figure out how to get ptr and size
		}

	}

	std::vector<vec<3, unsigned>> modified_chunks; //< Min corner of each 16^3 cube modified in this time step
};

static std::shared_ptr<gloom::network::Host> make_host(gloom::State& state)
{
	auto host = std::make_shared<gloom::network::Host>();

	host->on_connection = [&](int sock, State::Player::Session& sess) {
		std::cout << "player" << sock << " connected.\n";
		sess.id = gameplay::new_player_id(state);
		state.sessions.insert({sess.id, sess});
	};

	host->on_disconnection = [&](int sock, State::Player::Session& sess) {
		std::cout << "player" << sock << " disconnected\n";
		state.sessions.erase(sess.id);
	};

	host->on_packet = [&](int sock, State::Player::Session& sess) -> int {
		char datagram[0xFFFF];

 		auto& player = state.players[sess.id];

		// player_commands msg;
		auto bytes = read(sock, &datagram, sizeof(datagram));

		auto command = flatbuffers::GetRoot<gloom::api::commands::Player>(datagram);
		// msg.to_machine();
 		if (command->configure())
 		{
 			std::cout << "configure command" << std::endl;
			std::cout << "player " << command->configure()->name()->c_str() << " joined" << std::endl;
			player.name = std::string(command->configure()->name()->c_str());
 		}

 		if (command->control())
 		{
 			auto& c = *command->control()->control()->v();
 			auto& o = *command->control()->orientation()->v();
 			player.control = { c[0], c[1], c[2] };
 			player.orientation = { o[0], o[1], o[2], o[3] };
 		}

 		auto req = command->request();
 		if (req)
 		{
 			if (req->world_blocks())
 			{
 				for (auto corner : *req->world_blocks())
 				{
 					auto& c = *corner->v();
 					sess.requested_blocks.push_back({c[0], c[1], c[2]});
 				}
 			}
 		}

		// commands[p.index] = msg;

		return 0;
	};

	try
	{
		host->listen(1337);
	}
	catch(std::runtime_error e)
	{
		std::cerr << e.what() << std::endl;
		exit(-1);
	}

	return host;
}



} // namespace network

} // namespace gloom