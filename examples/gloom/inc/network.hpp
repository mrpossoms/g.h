#pragma once
#include <memory>

#include <g.h>

#include "state.hpp"
#include "gameplay.hpp"
#include "api/api_generated.h"

namespace gloom
{
namespace network
{

static std::shared_ptr<g::net::client> make_client(const std::string& hostname)
{
	auto client = std::make_shared<g::net::client>();

	client->connect(hostname, 1337);

	return client;
}

static std::shared_ptr<g::net::host<State::Player::Session>> make_host(gloom::State& state)
{
	auto host = std::make_shared<g::net::host<State::Player::Session>>();

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
		// player_commands msg;
		// auto bytes = read(sock, &msg, sizeof(msg));
		// msg.to_machine();
 
		// commands[p.index] = msg;

		return 0;
	};

	host->listen(1337);

	return host;
}

} // namespace network

} // namespace gloom