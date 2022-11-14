#include <g.h>

#include "state.hpp"
#include "gameplay.hpp"

using namespace gloom;

struct Gloom : public g::core
{
		g::net::client client;
		g::net::host<State::Player::Session> host;
		State state;

		Gloom()
		{
			// TODO
		}

		bool initialize() override
		{

			if (hosting)
			{
				host.on_connection = [&](int sock, State::Player::Session& sess) {
					std::cout << "player" << sock << " connected.\n";
					sess.id = gameplay::new_player_id(state);
					state.sessions.insert({sess.id, sess});
				};

				host.on_disconnection = [&](int sock, State::Player::Session& sess) {
					std::cout << "player" << sock << " disconnected\n";
					state.players.remove_at(p.index);
				};

				host.on_packet = [&](int sock, State::Player::Session& sess) -> int {
					player_commands msg;
					auto bytes = read(sock, &msg, sizeof(msg));
					msg.to_machine();

					commands[p.index] = msg;

					return 0;
				};
			}
			else
			{

			}
		}

		void update(float dt) override
		{

		}
};


int main(const int argc, const char* argv[])
{

	Gloom{}.start({ 
        .name = argv[0],
        .gfx = {
            .display = true,
            .width = 1024,
            .height = 768 
        }
    });

	return 0;
}