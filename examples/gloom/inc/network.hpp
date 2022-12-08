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

		auto config_command = gloom::commands::CreateConfigureCommand(builder, builder.CreateString("foob"));
		auto player_command = gloom::commands::CreatePlayer(builder, config_command);
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

	void send_states(const gloom::State& state)
	{
		flatbuffers::FlatBufferBuilder builder(0xFFFF);
	
		// Player info gets sent to all players every time
		gloom::state::Player players[state.players.size()];
		std::vector<const gloom::state::Player*> players_vec;
		std::vector<flatbuffers::Offset<gloom::state::PlayerConfig>> player_configs_vec;

		auto world_size = gloom::Vec3ui(state.world.voxels.size.cast<uint32_t>().v);
		auto world_info = gloom::state::world::Info(
			state.world.voxels.hash(),
			world_size
		);

		unsigned i = 0;
		for (auto& sock_sess_pair : state.players)
		{
			auto& id = sock_sess_pair.first;
			auto& player = sock_sess_pair.second;

			auto pos = gloom::Vec3f(player.position.v);
			auto vel = gloom::Vec3f(player.velocity.v);
			auto ori = gloom::Quat(player.orientation.v);
			players[i] = gloom::state::Player(id, pos, vel, ori);

			players_vec.push_back({&players[i]});
			i++;
			// TODO: player_configs_vec
		}

		gloom::state::gameBuilder gs_builder(builder);
		gs_builder.add_world(&world_info);
		gs_builder.add_players(builder.CreateVector(players_vec));
		gs_builder.add_player_configs(builder.CreateVector(player_configs_vec));

		for (auto& sock_sess_pair : state.players)
		{
			// TODO: player specific updates to game state here
			// TODO: send packet
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

		auto command = flatbuffers::GetRoot<gloom::commands::Player>(datagram);
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