#include <memory>

#include <g.h>

#include "state.hpp"
#include "gameplay.hpp"
#include "network.hpp"

using namespace gloom;

struct Gloom : public g::core
{
		std::shared_ptr<g::net::client> client;
		std::shared_ptr<g::net::host<State::Player::Session>> host;
		State state;

		Gloom(bool hosting)
		{
			std::cerr << "here?\n";

			// TODO
			if (hosting)
			{
				std::cerr << "hosting" << std::endl;
				host = gloom::network::make_host(state);
			}
			else
			{
				std::cerr << "connecting" << std::endl;
				client = gloom::network::make_client("localhost");
			}
		}

		bool initialize() override
		{
			return true;
		}

		void update(float dt) override
		{
			if (client)
			{
				client->update();
			}
			else if (host)
			{
				host->update();
			}
		}
};


int main(const int argc, const char* argv[])
{
	bool hosting = argc <= 1;

	std::cerr << "wtf?\n";

	Gloom{hosting}.start({ 
        .name = argv[0],
        .gfx = {
            .display = true,
            .width = 1024,
            .height = 768 
        }
    });

	return 0;
}