#include <g.h>


struct Gloom : public g::core
{
		g::net::client client;
		g::net::host<> host;

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