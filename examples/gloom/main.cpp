#include <memory>

#include <g.h>

#include "state.hpp"
#include "gameplay.hpp"
#include "network.hpp"
#include "renderers.hpp"

using namespace gloom;

struct Gloom : public g::core
{
		std::shared_ptr<g::net::client> client;
		std::shared_ptr<g::net::host<State::Player::Session>> host;
		State state;

		GameRenderer renderer;

		Gloom(bool hosting)
		{
			std::cerr << "here?\n";

			// TODO
			if (hosting)
			{
				std::cerr << "hosting" << std::endl;
				host = gloom::network::make_host(state);

				gloom::gameplay::load_level(state, "temple.vox");
			}
			else
			{
				std::cerr << "connecting" << std::endl;

				try
				{
					client = gloom::network::make_client("localhost", state);
				}
				catch (std::runtime_error e)
				{
					std::cerr << "Connection to server failed: " << std::endl << e.what() << std::endl;
					exit(-1);
				}

				std::cerr << "connected" << std::endl;
				client->listen();
			}
		}

		bool initialize() override
		{
			// TODO: temp
	        state.camera.on_input = [](fps_camera& cam, float dt){
	            static double xlast, ylast;
	            float sensitivity = 0.5f;
	            double xpos = 0, ypos = 0;
	            auto mode = glfwGetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR);

	            if (GLFW_CURSOR_DISABLED == mode)
	            {
	                glfwGetCursorPos(g::gfx::GLFW_WIN, &xpos, &ypos);
	            }

	            if (glfwGetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	            if (xlast != 0 || ylast != 0)
	            {
	                auto dx = xpos - xlast;
	                auto dy = ypos - ylast;
	                cam.pitch += (dy * dt * sensitivity);
	                cam.yaw += (-dx * dt * sensitivity);
	            }

	            xlast = xpos; ylast = ypos;

	            auto speed = cam.speed;

	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= (cam.touching_surface ? 5 : 1);
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.velocity += cam.body_forward() * speed;
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.velocity += cam.body_forward() * -speed;
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.velocity += cam.body_left() * speed;
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.velocity += cam.body_left() * -speed;
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.velocity += cam.body_up() * 5 * cam.touching_surface;
	            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	        
	            if (glfwGetMouseButton(g::gfx::GLFW_WIN, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	            {
	                glfwSetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	            }
	        };

	        state.camera.gravity = {0, 0, 0};

			return true;
		}

		void update(float dt) override
		{
			if (client)
			{
				// TODO: client game logic
			}
			else if (host)
			{
				gloom::gameplay::update_player_dynamics(state, dt);

		        // process input and update the velocities.
		        state.camera.pre_update(dt, 0);
		        state.camera.aspect_ratio(g::gfx::aspect());

		        // after velocities have been corrected, update the camera's position
		        state.camera.update(dt, 0);

				// TODO: host game logic
				renderer.draw(dt, state);
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