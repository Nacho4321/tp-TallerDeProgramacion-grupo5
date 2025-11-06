#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "constants.h"
#include "messages.h"
#include "socket.h"

class Protocol
{
private:
    Socket skt;
    std::vector<uint8_t> buffer;
    std::vector<uint8_t> readBuffer;

    //
    // Helpers para armar y desarmar mensajes
    //

    // Agrega un valor al buffer
    template <typename T>
    void appendValue(T value)
    {
        size_t old_size = buffer.size();
        buffer.resize(old_size + sizeof(T));
        std::memcpy(buffer.data() + old_size, &value, sizeof(T));
    }

    // Lee un valor del buffer y avanza el índice
    template <typename T>
    T readValue(const std::vector<uint8_t> &buffer, size_t &idx)
    {
        T value;
        std::memcpy(&value, buffer.data() + idx, sizeof(T));
        idx += sizeof(T);
        return value;
    }

    // Inserta tipos específicos en el buffer
    void insertUint16(std::uint16_t value);
    void insertUint32(std::uint32_t value);
    void insertFloat(float value);
    void insertInt(int value);

    // Extrae tipos específicos del buffer
    uint16_t exportUint16(const std::vector<uint8_t> &buffer, size_t &idx);
    uint32_t exportUint32(const std::vector<uint8_t> &buffer, size_t &idx);
    float exportFloat(const std::vector<uint8_t> &buffer, size_t &idx);
    int exportInt(const std::vector<uint8_t> &buffer, size_t &idx);
    bool exportBoolFromNitroStatus(const std::vector<uint8_t> &buffer, size_t &idx);

    // Lee player_id y game_id de la red y los coloca en msg
    void readClientIds(ClientMessage& msg);

    // Traduce un ClientMessage a bytes
    std::vector<std::uint8_t> encodeClientMessage(const ClientMessage &msg);
    std::vector<std::uint8_t> encodeServerMessage(ServerMessage& out);
    std::vector<std::uint8_t> encodeGameJoinedResponse(const GameJoinedResponse& response);

    // Helpers de encode internos para solo opcode
    std::vector<std::uint8_t> encodeOpcode(std::uint8_t opcode);

    // Helpers de receive
    ClientMessage receiveUpPressed();
    ClientMessage receiveUpRealesed();
    ClientMessage receiveDownPressed();
    ClientMessage receiveDownReleased();
    ClientMessage receiveLeftPressed();
    ClientMessage receiveLeftReleased();
    ClientMessage receiveRightPressed();
    ClientMessage receiveRightReleased();
    ClientMessage receiveCreateGame();
    ClientMessage receiveJoinGame();

    ServerMessage receivePositionsUpdate();
    GameJoinedResponse receiveGameJoinedResponse();

public:
    explicit Protocol(Socket &&socket) noexcept; // constructor que toma ownership del socket
    Protocol() = delete;
    ~Protocol() = default;
    Protocol(const Protocol &) = delete;
    Protocol &operator=(const Protocol &) = delete;

    // Recibe mensaje del socket en formato DecodedMessage
    ClientMessage receiveClientMessage();
    ServerMessage receiveServerMessage();
    
    // Lobby methods (cliente recibe respuesta del servidor)
    GameJoinedResponse receiveGameJoined();

    // Envía mensaje al socket
    void sendMessage(ServerMessage& out);
    void sendMessage(ClientMessage& out);
    void sendMessage(const GameJoinedResponse& response);

    void shutdown();
};

#endif
