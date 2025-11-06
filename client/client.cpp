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
      car_renderer("Game Renderer", 640, 480)
{
    sender.start();   // Start the sender thread
    receiver.start(); // Start the receiver thread
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
            if(input == "CREATE GAME"){
                input = "create_game";
            } else if(input.rfind("JOIN GAME", 0) == 0){
                // Extraer el ID de la partida
                size_t space_pos = input.find(' ');
                if (space_pos != std::string::npos) {
                    std::string game_id_str = input.substr(space_pos + 1);
                    input = "join_game " + game_id_str;
                } else {
                    std::cerr << "[Client] Invalid JOIN GAME command format." << std::endl;
                    continue; // Saltar este ciclo si el formato es inválido
                }
            }
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
            PlayerPositionUpdate pos_updated = latest_message.positions.front();

            CarPosition position = CarPosition{
                pos_updated.new_pos.new_X,
                pos_updated.new_pos.new_Y,
                float(pos_updated.new_pos.direction_x),
                float(pos_updated.new_pos.direction_y)};

            car_renderer.render(position);
        }

        SDL_Delay(16);
    }
}