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
    sender.start();  // Start the sender thread
    receiver.start();  // Start the receiver thread
}

Client::~Client() {
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
        
        if (input == "QUIT") {
            connected = false;
        } else if (!input.empty()) {
            std::cout << "[Client] Sending: " << input << std::endl;
            outgoing_messages.push(input);
        }

        DecodedMessage message;
        incoming_messages.try_pop(message); // Try to pop a message from the incoming queue

        CarPosition position;         // This position should come from server messages
        position.x = 960;               // Dummy values for now
        position.y = 540;
        position.directionX = 0;
        position.directionY = 0;

        car_renderer.render(position);

        SDL_Delay(16);  // ~60 FPS
    }
}