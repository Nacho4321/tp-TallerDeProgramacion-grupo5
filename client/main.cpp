
#include <iostream>
#include <exception>

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>

using namespace SDL2pp;

#include <iostream>
#include <string>

#include "../common/socket.h"

#include "client.h"
#define ADDRESS_ARG 1
#define PORT_ARG 2
#define CANT_ARGS 3
#define CLIENT_ERROR "Error in client: "
#define CLIENT_PARAMS " <address> <port>\n"
int main(int argc, const char *argv[])
{
	try
	{
		if (argc != CANT_ARGS)
		{
			std::cerr << "Use: " << argv[0] << CLIENT_PARAMS;
			return 1;
		}
		Client client(argv[ADDRESS_ARG], argv[PORT_ARG]);
		client.start();

		// Initialize SDL library
		SDL sdl(SDL_INIT_VIDEO);

		// Create main window: 640x480 dimensions, resizable, "SDL2pp demo" title
		Window window("SDL2pp demo",
					  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					  640, 480,
					  SDL_WINDOW_RESIZABLE);

		// Create accelerated video renderer with default driver
		Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

		// Clear screen
		renderer.Clear();

		// Show rendered frame
		renderer.Present();

		// 5 second delay
		SDL_Delay(5000);
	}
	catch (const std::exception &e)
	{
		std::cerr << CLIENT_ERROR << e.what() << std::endl;
		return 1;
	}
	// Here all resources are automatically released and library deinitialized
	return 0;
}