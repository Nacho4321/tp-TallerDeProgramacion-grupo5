#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>

// Mensaje que el server va a manejar en su loop
struct OutgoingMessage {
    std::string cmd;
};

struct IncomingMessage {
    std::string cmd;
    int client_id;
};
#endif
