#include <g.h>


struct player
{
	player() = default;

	int hp;
};


struct chat_msg : g::net::msg
{
	void to_network() { id = htonl(id); }

	void to_machine() { id = ntohl(id); }

	char buf[128];
	int id;
};


struct my_core : public g::core
{
	g::net::host<player> host;

	virtual bool initialize()
	{
		host.listen(1337);

		host.on_connection = [&](int sock, player& p) {
			std::cout << "player" << sock << " connected.\n";
			p.hp = 100;
		};

		host.on_disconnection = [&](int sock, player& p) {
			std::cout << "player" << sock << " disconnected\n";
		};

		host.on_packet = [&](int sock, player& p) -> int {
			chat_msg msg;
			auto bytes = read(sock, &msg, sizeof(msg));
			msg.to_machine();
			msg.id = sock;

			std::cout << "user" << sock << ": " << std::string(msg.buf) << std::endl;
			msg.to_network();

			for (auto pair : host.sockets)
			{
				// don't send the message back to the sender
				// if (pair.first == sock) { continue; }

				send(pair.first, (const char*)&msg, bytes, 0);
			}

			return 0;
		};

		return true;
	}

	virtual void update(float dt)
	{
		// host.update();

		std::cout << "server time: " << time(NULL) << " dt:"<< dt <<"\n";
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
};


int main (int argc, const char* argv[])
{
	my_core server;

	server.start({ "02.net_host", false, 0, 0 });

	return 0;
}
