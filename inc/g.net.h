#pragma once

#include "g.utils.h"
#include <sys/types.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#if defined(_WIN32)
#include <Windows.h>
#include <winsock.h>
#endif

#define SHA1HANDSOFF

#include <stdio.h>
#include <string.h>

/* for uint32_t */
#include <stdint.h>

#ifdef _WIN32
using socket_t = SOCKET;

static bool init_winsock()
{
	WSADATA wsa_data = {0};
	auto result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0) 
	{
		std::cerr << "WSAStartup failed: " << result << std::endl;
		return false;
	}
	else
	{
		return true;
	}
}
#else
using socket_t = int;
#define INVALID_SOCKET (-1)
#endif

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#if BYTE_ORDER == LITTLE_ENDIAN
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#elif BYTE_ORDER == BIG_ENDIAN
#define blk0(i) block->l[i]
#else
#error "Endianness not defined!"
#endif
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

static void SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
)
{
    uint32_t a, b, c, d, e;

    typedef union
    {
        unsigned char c[64];
        uint32_t l[16];
    } CHAR64LONG16;

#ifdef SHA1HANDSOFF
    CHAR64LONG16 block[1];      /* use array to appear as a pointer */

    memcpy(block, buffer, 64);
#else
    /* The following had better never be used because it causes the
     * pointer-to-const buffer to be cast into a pointer to non-const.
     * And the result is written through.  I threw a "const" in, hoping
     * this will cause a diagnostic.
     */
    CHAR64LONG16 *block = (const CHAR64LONG16 *) buffer;
#endif
    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a, b, c, d, e, 0);
    R0(e, a, b, c, d, 1);
    R0(d, e, a, b, c, 2);
    R0(c, d, e, a, b, 3);
    R0(b, c, d, e, a, 4);
    R0(a, b, c, d, e, 5);
    R0(e, a, b, c, d, 6);
    R0(d, e, a, b, c, 7);
    R0(c, d, e, a, b, 8);
    R0(b, c, d, e, a, 9);
    R0(a, b, c, d, e, 10);
    R0(e, a, b, c, d, 11);
    R0(d, e, a, b, c, 12);
    R0(c, d, e, a, b, 13);
    R0(b, c, d, e, a, 14);
    R0(a, b, c, d, e, 15);
    R1(e, a, b, c, d, 16);
    R1(d, e, a, b, c, 17);
    R1(c, d, e, a, b, 18);
    R1(b, c, d, e, a, 19);
    R2(a, b, c, d, e, 20);
    R2(e, a, b, c, d, 21);
    R2(d, e, a, b, c, 22);
    R2(c, d, e, a, b, 23);
    R2(b, c, d, e, a, 24);
    R2(a, b, c, d, e, 25);
    R2(e, a, b, c, d, 26);
    R2(d, e, a, b, c, 27);
    R2(c, d, e, a, b, 28);
    R2(b, c, d, e, a, 29);
    R2(a, b, c, d, e, 30);
    R2(e, a, b, c, d, 31);
    R2(d, e, a, b, c, 32);
    R2(c, d, e, a, b, 33);
    R2(b, c, d, e, a, 34);
    R2(a, b, c, d, e, 35);
    R2(e, a, b, c, d, 36);
    R2(d, e, a, b, c, 37);
    R2(c, d, e, a, b, 38);
    R2(b, c, d, e, a, 39);
    R3(a, b, c, d, e, 40);
    R3(e, a, b, c, d, 41);
    R3(d, e, a, b, c, 42);
    R3(c, d, e, a, b, 43);
    R3(b, c, d, e, a, 44);
    R3(a, b, c, d, e, 45);
    R3(e, a, b, c, d, 46);
    R3(d, e, a, b, c, 47);
    R3(c, d, e, a, b, 48);
    R3(b, c, d, e, a, 49);
    R3(a, b, c, d, e, 50);
    R3(e, a, b, c, d, 51);
    R3(d, e, a, b, c, 52);
    R3(c, d, e, a, b, 53);
    R3(b, c, d, e, a, 54);
    R3(a, b, c, d, e, 55);
    R3(e, a, b, c, d, 56);
    R3(d, e, a, b, c, 57);
    R3(c, d, e, a, b, 58);
    R3(b, c, d, e, a, 59);
    R4(a, b, c, d, e, 60);
    R4(e, a, b, c, d, 61);
    R4(d, e, a, b, c, 62);
    R4(c, d, e, a, b, 63);
    R4(b, c, d, e, a, 64);
    R4(a, b, c, d, e, 65);
    R4(e, a, b, c, d, 66);
    R4(d, e, a, b, c, 67);
    R4(c, d, e, a, b, 68);
    R4(b, c, d, e, a, 69);
    R4(a, b, c, d, e, 70);
    R4(e, a, b, c, d, 71);
    R4(d, e, a, b, c, 72);
    R4(c, d, e, a, b, 73);
    R4(b, c, d, e, a, 74);
    R4(a, b, c, d, e, 75);
    R4(e, a, b, c, d, 76);
    R4(d, e, a, b, c, 77);
    R4(c, d, e, a, b, 78);
    R4(b, c, d, e, a, 79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
#ifdef SHA1HANDSOFF
    memset(block, '\0', sizeof(block));
#endif
}


/* SHA1Init - Initialize new context */

static void SHA1Init(
    SHA1_CTX * context
)
{
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}


/* Run your data through this. */

static void SHA1Update(
    SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
)
{
    uint32_t i;

    uint32_t j;

    j = context->count[0];
    if ((context->count[0] += len << 3) < j)
        context->count[1]++;
    context->count[1] += (len >> 29);
    j = (j >> 3) & 63;
    if ((j + len) > 63)
    {
        memcpy(&context->buffer[j], data, (i = 64 - j));
        SHA1Transform(context->state, context->buffer);
        for (; i + 63 < len; i += 64)
        {
            SHA1Transform(context->state, &data[i]);
        }
        j = 0;
    }
    else
        i = 0;
    memcpy(&context->buffer[j], &data[i], len - i);
}


/* Add padding and return the message digest. */

static void SHA1Final(
    unsigned char digest[20],
    SHA1_CTX * context
)
{
    unsigned i;

    unsigned char finalcount[8];

    unsigned char c;

#if 0    /* untested "improvement" by DHR */
    /* Convert context->count to a sequence of bytes
     * in finalcount.  Second element first, but
     * big-endian order within element.
     * But we do it all backwards.
     */
    unsigned char *fcp = &finalcount[8];

    for (i = 0; i < 2; i++)
    {
        uint32_t t = context->count[i];

        int j;

        for (j = 0; j < 4; t >>= 8, j++)
            *--fcp = (unsigned char) t}
#else
    for (i = 0; i < 8; i++)
    {
        finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);      /* Endian independent */
    }
#endif
    c = 0200;
    SHA1Update(context, &c, 1);
    while ((context->count[0] & 504) != 448)
    {
        c = 0000;
        SHA1Update(context, &c, 1);
    }
    SHA1Update(context, finalcount, 8); /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++)
    {
        digest[i] = (unsigned char)
            ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
    /* Wipe variables */
    memset(context, '\0', sizeof(*context));
    memset(&finalcount, '\0', sizeof(finalcount));
}

static void SHA1(
    char *hash_out,
    const char *str,
    unsigned len)
{
    SHA1_CTX ctx;
    unsigned int ii;

    SHA1Init(&ctx);
    for (ii=0; ii<len; ii+=1)
        SHA1Update(&ctx, (const unsigned char*)str + ii, 1);
    SHA1Final((unsigned char *)hash_out, &ctx);
    hash_out[20] = '\0';
}

namespace g
{

struct net
{
	struct msg
	{
		virtual void to_network() = 0;
		virtual void to_machine() = 0;
	};

	/**
	 * @brief      A host object is used as a manager of network connections
	 * this includes listening for new connections, and incomming messages.
	 * When a connection is established a new instance of the template param
	 * T is instantiated and paired to that connection
	 *
	 * @tparam     T     Class which will be instantiated for each connection.
	 */
	template<typename T>
	struct host
	{

		std::function<void(socket_t socket, T& client)> on_connection;
		std::function<void(socket_t socket, T& client)> on_disconnection;
		std::function<int(socket_t socket, T& client)> on_packet;
		std::unordered_map<socket_t, T> sockets;
		std::unordered_set<socket_t> senders;
		std::thread listen_thread;
		std::atomic<bool> running = {true};
		int listen_socket;

		host()
		{
#ifdef _WIN32
			static bool winsock_inited;

			if (!winsock_inited)
			{
				winsock_inited = init_winsock();
			}
#endif
		}

		~host()
		{
			running = false;
			::close(listen_socket);
			listen_thread.join();
		}

		/**
		 * @brief      Calling this method will bind a socket for accepting connections
		 * to the provided port, then start a seperate thread to manage data received
		 * from clients, new connections, and disconnection events. This method can
		 * throw std::runtime_error exceptions for a number of reasons.
		 *
		 * @param[in]  port  The port to listen on
		 */
		void listen(short port)
		{
#ifdef _WIN32
			int protocol = IPPROTO_TCP;
#else
			int protocol = 0;
#endif

			listen_socket = ::socket(AF_INET, SOCK_STREAM, protocol);

			if (listen_socket == INVALID_SOCKET)
			{
				throw std::runtime_error("listen sock creation failed: " + std::string(strerror(errno)));
			}

			// allow port reuse for quicker restarting
#ifdef __linux__
			int use = 1;
			if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&use, sizeof(use)))
			{
				close(listen_socket);
				throw std::runtime_error("Setting SO_REUSEPORT to listen socket failed");
			}
#endif

			struct sockaddr_in name = {};
			name.sin_family = AF_INET;
			name.sin_port = htons(port);
			name.sin_addr.s_addr = htonl(INADDR_ANY);

			// bind the listening sock to port number
			if (bind(listen_socket, (const struct sockaddr*)&name, sizeof(name)))
			{
				close(listen_socket);
				throw std::runtime_error("listen sock bind failed");
			}

			// begin listening
			if(::listen(listen_socket, 1))
			{
				close(listen_socket);
				throw std::runtime_error("listen sock listen failed");
			}

			listen_thread = std::thread([&]{
				while(running)
				{
					update();
				}
			});
		}

		bool is_client_ws(int sock)
		{
			char buf[1024] = {};
			std::unordered_map<std::string, std::string> headers;
			auto bytes = recv(sock, buf, sizeof(buf), MSG_PEEK);

			auto lines = std::string(buf);
			int i = 0;
			for (auto line : g::utils::split(lines, "\r\n"))
			{
				std::cerr << "[" << line << "]\n";
				if (i == 0 && std::string::npos == line.find("GET")) { return false; }
				else
				{
					std::string key;
					for (auto part : g::utils::split(line, ": "))
					{
						if (key.length() == 0) { key = part; }
						else
						{
							headers[key] = part;
							break;
						}
					}
				}
				i++;
			}

			const auto sec_key = "Sec-WebSocket-Key";
			if (headers.count(sec_key))
			{
				const auto SHA_DIGEST_LENGTH = 20;
				char* next = buf;
				auto key = headers[sec_key] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				char hash[SHA_DIGEST_LENGTH + 1];
				::SHA1(hash, (const char*)key.c_str(), key.length());
				char sec_accept[29] = {};
				g::utils::base64_encode(sec_accept, hash, SHA_DIGEST_LENGTH);

				{
					std::string ex_key = "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
					char ex_sha[SHA_DIGEST_LENGTH];
					::SHA1(ex_sha, (const char*)ex_key.c_str(), ex_key.length());
					char ex_accept[28];
					g::utils::base64_encode(ex_accept, ex_sha, 20);

					assert(0 == memcmp(ex_accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", 28));
				}

				next += sprintf(next, "HTTP/1.1 101 Switching Protocols\r\n");
				next += sprintf(next, "Upgrade: websocket\r\n");
				next += sprintf(next, "Connection: Upgrade\r\n");
				next += sprintf(next, "Sec-WebSocket-Accept: %20s\r\n", sec_accept);
				// next += sprintf(next, "Sec-WebSocket-Version: %s\r\n", headers["Sec-WebSocket-Version"].c_str());
				// next += sprintf(next, "Content-Length: 0\r\n");
				next += sprintf(next, "\r\n");
				printf(">> RESPONSE\n");
				write(1, buf, (next - buf));
				send(sock, buf, (next - buf), 0);
				std::cout << "sha: " << hash << "\n";
			}
			else
			{
				return false;
			}

			// purge the http request we just got
			read(sock, buf, bytes);

			return true;
		}

		void update()
		{
			socket_t max_sock = listen_socket;
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(listen_socket, &rfds);

			// for each client socket check connection status and set it
			for (auto& pair : sockets)
			{
				int sock = pair.first;

				FD_SET(sock, &rfds);
				max_sock = std::max<socket_t>(sock, max_sock);
			}

			struct timeval timeout = { 0, 500000 };

			switch (select(max_sock + 1, &rfds, NULL, NULL, &timeout))
			{
				case -1: { /* select error, possible disconnect */ }
				case 0: { /* timeout occured */ }
				default:
				{
					std::vector<socket_t> disconnected_socks;

					// check all client sockets to see if any have messages
					for (auto& pair : sockets)
					{
						auto sock = pair.first;
						if (!FD_ISSET(sock, &rfds)) { continue; }

						// check the connection status of the client socket
						// otherwise, pass the message over to the lambda
						char c;
						auto res = recv(sock, &c, 1, MSG_PEEK);
#ifdef _WIN32
						switch (res)
						{
							case -1:
								// Error, try again (maybe)
								// some additional logic is needed here
							{
								// ref: https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
								switch (WSAGetLastError())
								{
									case WSANOTINITIALISED:
									case WSAENETDOWN:
									case WSAEFAULT:
									case WSAENOTCONN:
									case WSAENETRESET:
									case WSAENOTSOCK:
									case WSAEOPNOTSUPP:
									case WSAESHUTDOWN:
									case WSAEINVAL:
									case WSAECONNABORTED:
									case WSAETIMEDOUT:
									case WSAECONNRESET:
										// Connection has closed
										on_disconnection(sock, sockets[sock]);
										// close(sock); // Closing 
										disconnected_socks.push_back(sock);

										// if the last connection just dropped, just return.
										if (sockets.size() == 0) { return; }
										break;
									case WSAEINTR:
									case WSAEINPROGRESS:
									case WSAEWOULDBLOCK:
									case WSAEMSGSIZE:
										break;
								}
							}
								break;
							case 0:
								// Connection has closed
								on_disconnection(sock, sockets[sock]);
								close(sock);
								disconnected_socks.push_back(sock);

								// if the last connection just dropped, just return.
								if (sockets.size() == 0) { return; }
								break;

							default:
								if (senders.count(sock) == 0)
								{
									// this socket hasn't sent anything yet
									// lets check to see if it's a WS.
									if (is_client_ws(sock))
									{
										senders.insert(sock);
										break;
									}
								}
								on_packet(sock, pair.second);
								senders.insert(sock);
								break;
						}
#else
						switch (res)
						{
							case -1:
							case 0:
								// these errors only mean there was nothing for us to
								// read at this moment. Not that the connection is
								// broken
								if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }

								on_disconnection(sock, sockets[sock]);
								close(sock);
								disconnected_socks.push_back(sock);

								// if the last connection just dropped, just return.
								if (sockets.size() == 0) { return; }
								break;
							default:
								if (senders.count(sock) == 0)
								{
									// this socket hasn't sent anything yet
									// lets check to see if it's a WS.
									if (is_client_ws(sock))
									{
										senders.insert(sock);
										break;
									}
								}
								on_packet(sock, pair.second);
								senders.insert(sock);
								break;
						}
#endif
					}

					// clean up those that have disconnected
					for (auto sock : disconnected_socks)
					{
						sockets.erase(sock);
						senders.erase(sock);
					}

					if (FD_ISSET(listen_socket, &rfds))
					{
						struct sockaddr_in client_name = {};
#ifdef _WIN32
						int client_name_len = sizeof(client_name);
#else
						socklen_t client_name_len = 0;
#endif
						socket_t sock = accept(
							listen_socket,
							(struct sockaddr*)&client_name,
							&client_name_len
						);

						if (sock != INVALID_SOCKET)
						{
						
	#ifdef __linux__
							int one = 1, five = 5;
							setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
							setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &five, sizeof(five));
							setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &one, sizeof(one));
							setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &five, sizeof(five));
	#endif
							sockets[sock] = {};

							on_connection(sock, sockets[sock]);
						}
					}
				}
			}
		}
	};

	struct client
	{
		std::function<int(socket_t socket)> on_connection;
		std::function<int(socket_t socket)> on_packet;
		std::function<void(socket_t socket)> on_disconnection;
		std::thread listen_thread;
		socket_t socket;
		volatile bool is_connected = false;

		client()
		{
#ifdef _WIN32
			static bool winsock_inited;

			if (!winsock_inited)
			{
				winsock_inited = init_winsock();
			}
#endif
		}

		bool connect(const std::string& hostname, short port)
		{
			// resolve host
			auto host = gethostbyname(hostname.c_str());
			if (host == nullptr)
			{
				return false;
			}

			// print resolved host information
			std::cerr << "hostname: " << host->h_name << std::endl;
			std::cerr << "address_type: " << host->h_addrtype << " address_len: " << host->h_length << std::endl;
			std::cerr << "addresses:" << std::endl;
			for (unsigned i = 0; host->h_addr_list[i] != nullptr; i++)
			{
				std::cerr << "\t";
				for (unsigned j = 0; j < 4; j++)
				{
					std::cerr << static_cast<unsigned>(host->h_addr_list[i][j]);
					if (j < 3) { std::cerr << "."; }
				}
				std::cerr << std::endl;
			}


			struct sockaddr_in host_addr = {};
			// fill in host_addr with resolved info
			memcpy(
				(char*)&host_addr.sin_addr.s_addr,
				(char *)host->h_addr_list[0],
				host->h_length
			);
			host_addr.sin_port   = htons(port);
			host_addr.sin_family = AF_INET;

			socket = ::socket(AF_INET, SOCK_STREAM, 0);
			// fcntl(socket, F_SETFL, O_NONBLOCK);

			auto res = ::connect(socket, (const sockaddr*)&host_addr, sizeof(host_addr));

#ifdef __linux__
			int one = 1, five = 5;
			setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &five, sizeof(five));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &one, sizeof(one));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &five, sizeof(five));
#endif

			if (0 == res)
			{
				if (nullptr != on_connection)
				{
					on_connection(socket);
				}

				is_connected = true;
				return true;
			}

			throw std::runtime_error("sock connection failed: " + std::string(strerror(errno)));

			// otherwise, cleanup and return false.
			close(socket);
			return false;
		}


		void listen()
		{
			if (listen_thread.joinable()) { listen_thread.join(); }
			listen_thread = std::thread([&]{
				while(is_connected)
				{
					update();
				}
			});
		}


		void update()
		{
			char c;
			switch (recv(socket, &c, sizeof(c), MSG_PEEK))
			{
				case -1:
				case 0:
					// these errors only mean there was nothing for us to
					// read at this moment. Not that the connection is
					// broken
					if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }

					is_connected = false;
					on_disconnection(socket);
					close(socket);
					socket = -1;
					break;
				default:
					on_packet(socket);
					break;
			}
		}
	};
};
}
