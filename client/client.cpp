#include "client.h"
#include <iostream>
#include <string>
#include <SDL2/SDL.h>

Client::Client(const char *address, const char *port)
    : protocol(ini_protocol(address, port)),
      connected(true),
      handler(),
      outgoing_messages(),
      sender(protocol, outgoing_messages),
      receiver(protocol, incoming_messages),
      game_renderer("Game Renderer", 640, 480)  
{
    sender.start();  
    receiver.start();
}

Client::~Client()
{
    sender.stop();
    sender.join();
    receiver.stop();
    receiver.join();
}

void Client::start()
{
    while (connected)
    {
        std::string input = handler.receive();
        if (input == "QUIT")
        {
            connected = false;
        }
        else if (!input.empty())
        {
            std::cout << "[Client] Sending: " << input << std::endl;
            outgoing_messages.push(input);
        }

        ServerMessage message;
        ServerMessage latest_message;
        bool got_message = false;

        while (incoming_messages.try_pop(message))
        {
            latest_message = message;
            got_message = true;
        }

        if (got_message && !latest_message.positions.empty())
        {
            PlayerPositionUpdate main_pos = latest_message.positions.front();
            CarPosition mainCarPosition = CarPosition{
                main_pos.new_pos.new_X,
                main_pos.new_pos.new_Y,
                float(main_pos.new_pos.direction_x),
                float(main_pos.new_pos.direction_y)
            };

            std::vector<CarPosition> otherCars;
            for (size_t i = 1; i < latest_message.positions.size(); ++i)
            {
                PlayerPositionUpdate& pos = latest_message.positions[i];
                otherCars.push_back(CarPosition{
                    pos.new_pos.new_X,
                    pos.new_pos.new_Y,
                    float(pos.new_pos.direction_x),
                    float(pos.new_pos.direction_y)
                });
            }

            game_renderer.render(mainCarPosition, otherCars);
        }

        SDL_Delay(16);
    }
}