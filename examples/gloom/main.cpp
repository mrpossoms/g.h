#include <g.h>

#include "state.hpp"


struct Gloom : public g::core
{
		g::net::client client;
		g::net::host<gloom::Player::Session> host;

		Gloom()
		{
			// TODO
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