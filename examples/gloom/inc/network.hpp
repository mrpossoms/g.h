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
	
		std::vector<gloom::state::Player> players_vec;
		std::vector<gloom::state::PlayerConfig> player_configs_vec;

		for (auto& sock_sess_pair : state.players)
		{
			auto& id = sock_sess_pair.first;
			auto& player = sock_sess_pair.second;

			players_vec.push_back(gloom::state::CreatePlayer(id, {0,0,0}, {0,0,0}, {0,0,0,1}));
		}
	}
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