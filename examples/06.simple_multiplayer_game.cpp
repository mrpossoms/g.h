#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

#ifdef __EMSCRIPTEN__
const std::string vs_tex_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying lowp vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_tex_src =
"varying lowp vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";

const std::string fs_white_src =
"void main (void) {"
"gl_FragColor = vec4(1.0);"
"}";
#else
const std::string vs_tex_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"gl_PointSize = 2.0/(gl_Position.z);"
"}";

const std::string fs_tex_src =
"varying vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";

const std::string fs_white_src =
"void main (void) {"
"gl_FragColor = vec4(1.0);"
"}";
#endif

struct player_commands : g::net::msg
{
	float ang_vel;
	vec<2> thrust;
	uint8_t shooting;

	void to_network() {  }
	void to_machine() {  }
};

struct mover
{
	vec<2> position, velocity;

	void update(float dt)
	{
		position += velocity * dt;
	}

	void to_network() {  }
	void to_machine() {  }
};

struct player_info
{
	uint8_t index;
};

struct bullet : public mover
{
	float life;
	uint8_t owner_idx;

	bullet() = default;

	bullet(const vec<2>& pos, const vec<2>& vel, float life, uint8_t owner_idx)
	{
		position = pos;
		velocity = vel;
		this->life = life;
		this->owner_idx = owner_idx;
	}
};

struct player : public mover
{
	float angle;
	float cool_down;
	uint8_t hp;
	uint8_t shooting;

	void to_network() {  }
	void to_machine() {  }

	void respawn()
	{
		velocity = {};
		position = {};
		hp = 20;
	}
};

struct game_state_hdr
{
	uint8_t your_idx;
	uint8_t player_count;
	uint8_t bullet_count;

	void to_network() {  }
	void to_machine() {  }
};

struct game_state
{
	g::bounded_list<player, 10> players;
	g::bounded_list<bullet, 100> bullets;
};

struct zappers : public g::core
{
	std::mutex state_lock;
	bool is_host = false;
	int my_index = 0;
	g::asset::store assets;
	std::unordered_map<int, player_commands> commands;
	game_state state;

	g::net::client client;
	g::net::host<player_info> host;

	g::gfx::shader basic_shader, star_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::mesh<g::gfx::vertex::pos> stars;
	g::game::camera_orthographic cam;

	virtual bool initialize()
	{
		{ // graphics init
			basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_tex_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_tex_src)
												   .create();
			star_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_tex_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_white_src)
												   .create();
			plane = g::gfx::mesh_factory::plane();


			std::vector<g::gfx::vertex::pos> star_verts;
			for (int i = 0; i < 10000; i++)
			{
				star_verts.push_back({{ rand() % 128 - 64.f, rand() % 128 - 64.f, -rand() % 10 + 1.0f}});
			}

			stars = g::gfx::mesh_factory::empty_mesh<g::gfx::vertex::pos>().set_vertices(star_verts);

			glEnable(GL_PROGRAM_POINT_SIZE);
		}

		{ // server behaviors
			host.on_connection = [&](int sock, player_info& p) {
				std::cout << "player" << sock << " connected.\n";
				p.index = state.players.size();
				state.players.push_back({});
			};

			host.on_disconnection = [&](int sock, player_info& p) {
				std::cout << "player" << sock << " disconnected\n";
				state.players.remove_at(p.index);
			};

			host.on_packet = [&](int sock, player_info& p) -> int {
				player_commands msg;
				auto bytes = read(sock, &msg, sizeof(msg));
				msg.to_machine();

				commands[p.index] = msg;

				return 0;
			};
		}

		{ // client behaviors
			client.on_disconnection = [&](int sock) {
				std::cout << "you have been disconnected\n";
			};

			client.on_packet = [&](int sock) -> int {
				state_lock.lock();
				game_state_hdr msg;
				read(sock, &msg, sizeof(msg));
				msg.to_machine();
				my_index = msg.your_idx;

				// read all players
				state.players.clear();
				for (auto i = 0; i < msg.player_count; i++)
				{
					player p;
					read(sock, &p, sizeof(p));
					p.to_machine();
					state.players.push_back(p);
				}

				// read all bullets
				state.bullets.clear();
				for (auto i = 0; i < msg.bullet_count; i++)
				{
					bullet b;
					read(sock, &b, sizeof(b));
					b.to_machine();
					state.bullets.push_back(b);
				}

				state_lock.unlock();

				return 0;
			};
		}

		cam.width = 512;
		cam.height = 512;

		glDisable(GL_CULL_FACE);

		return true;
	}

	virtual void update(float dt)
	{
		if (is_host)
		{
			for (int i = 0; i < state.players.size(); i++)
			{
				auto& cmd = commands[i];
				auto& player = state.players[i];
				auto q = quat<>::from_axis_angle({0, 0, 1}, player.angle);
				auto thrust = q.rotate({cmd.thrust[0], cmd.thrust[1], 0}) * dt;
				player.velocity += thrust.slice<2>(0);
				player.angle += cmd.ang_vel * dt;
				player.cool_down -= dt;
				player.update(dt);

				if (cmd.shooting && player.cool_down <= 0)
				{
					auto vel = quat<>::from_axis_angle({0, 0, 1}, player.angle)
							   .rotate({0, 10, 0})
							   .slice<2>();

					state.bullets.push_back(bullet{
						player.position,
						player.velocity + vel,
						2,
						(uint8_t)i
					});

					player.cool_down = 0.25;
				}

				for (int j = 0; j < state.bullets.size(); j++)
				{
					auto& bullet = state.bullets[j];
					if (bullet.owner_idx == i) { continue; }

					if ((bullet.position - player.position).magnitude() <= 0.1)
					{
						player.hp -= 1;
						bullet.life = 0;
					}
				}

				if (player.hp <= 0) player.respawn();
			}

			for (int i = 0; i < state.bullets.size(); i++)
			{
				auto& bullet = state.bullets[i];
				bullet.update(dt);
				bullet.life -= dt;
				if (bullet.life <= 0)
				{
					state.bullets.remove_at(i);
				}
			}

			for (auto& player : host.sockets)
			{
				int sock = player.first;
				game_state_hdr hdr = {
					player.second.index,
					(uint8_t)state.players.size(),
					(uint8_t)state.bullets.size(),
				};

				write(sock, &hdr, sizeof(hdr));

				for (auto i = 0; i < state.players.size(); i++)
				{
					auto p = state.players[i];
					write(sock, &p, sizeof(p));
					p.to_machine();
				}

				for (auto i = 0; i < state.bullets.size(); i++)
				{
					auto p = state.bullets[i];
					write(sock, &p, sizeof(p));
					p.to_machine();
				}
			}
		}
		else
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
					std::cerr << "connection failure, switching to host\n";
					is_host = true;
					host.listen(1337);
					state.players.push_back({});
				}
			}
		}

		player_commands cmd = {};
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cmd.thrust += { 0,  1 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cmd.thrust += { 0, -1 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cmd.thrust += {-1,  0 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cmd.thrust += { 1,  0 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cmd.ang_vel = 1;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cmd.ang_vel = -1;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cmd.shooting = 1;

		if (is_host)
		{
			commands[0] = cmd;
		}
		else
		{
			write(client.socket, &cmd, sizeof(cmd));
		}


		state_lock.lock();

		cam.position = {-state.players[my_index].position[0], -state.players[my_index].position[1], 0};
		cam.orientation = quat<>::from_axis_angle({0, 0, 1}, state.players[my_index].angle);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto model = mat4::scale({1000, 1000, 1000}) * mat4::translation({0, 0, -10.1});
		plane.using_shader(basic_shader)
		.set_camera(cam)
		["u_model"].mat4(model)
		["u_tex"].texture(assets.tex("nebula.png"))
		.draw<GL_TRIANGLE_FAN>();

		stars.using_shader(star_shader)
		.set_camera(cam)
		["u_model"].mat4(mat4::I())
		.draw<GL_POINTS>();

		for (auto player : state.players)
		{
			auto model = (mat4::rotation({0, 0, 1}, player.angle) * mat4::scale({0.25, 0.25, 0.25})) * mat4::translation({player.position[0], player.position[1], -1});
			plane.using_shader(basic_shader)
			.set_camera(cam)
			["u_model"].mat4(model)
			["u_tex"].texture(assets.tex("ship.png"))
			.draw<GL_TRIANGLE_FAN>();
		}

		for (auto bullet : state.bullets)
		{
			auto model = mat4::scale({0.05, 0.05, 0.05}) * mat4::translation({bullet.position[0], bullet.position[1], -1});
			plane.using_shader(basic_shader)
			.set_camera(cam)
			["u_model"].mat4(model)
			["u_tex"].texture(assets.tex("bolt.png"))
			.draw<GL_TRIANGLE_FAN>();
		}

		state_lock.unlock();
	}
};


int main (int argc, const char* argv[])
{
	zappers game;

	game.start({ "zappers", true, 512, 512 });

	return 0;
}
