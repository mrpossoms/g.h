#include <g.h>
#include <stdio.h>

struct chat_msg : g::net::msg
{
	void to_network() { id = htonl(id); }

	void to_machine() { id = ntohl(id); }

	char buf[128];
	int id;
};

struct my_core : public g::core
{
	g::net::client client;

	virtual bool initialize()
	{
		client.on_disconnection = [&](int sock) {
			std::cout << "you have been disconnected\n";
		};

		client.on_packet = [&](int sock) -> int {
			chat_msg msg;
			read(sock, &msg, sizeof(msg));
			msg.to_machine();

			std::cout << "user" << msg.id << ": " << std::string(msg.buf) <<std::endl;

			return 0;
		};

		return true;
	}

	virtual void update(float dt)
	{
		if (!client.is_connected)
		{
			std::cerr << "Connecting...\n";

			if (client.connect("127.0.0.1", 1337))
			{
				std::cerr << "connected!\n";
				client.listen();				
			}
			else
			{
				std::cerr << "connection failure\n";
			}
		}

		chat_msg out;
		fgets(out.buf, sizeof(out.buf), stdin);
		out.to_network();
		send(client.socket, (const char*)&out, sizeof(out), 0);
	}
};


int main (int argc, const char* argv[])
{
	my_core client;

	client.start({ "03.net_client", false, 0, 0 });

	return 0;
}
