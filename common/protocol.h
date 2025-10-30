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

    // Traduce un comando a bytes
    std::vector<std::uint8_t> encodeCommand(const std::string &cmd);
    std::vector<std::uint8_t> encodeServerMessage(ServerMessage& out);

    // Helpers de encode
    std::vector<std::uint8_t> encodeClientMessage(std::uint8_t opcode);

    // Helpers de receive
    ClientMessage receiveUpPressed();
    ClientMessage receiveUpRealesed();
    ClientMessage receiveDownPressed();
    ClientMessage receiveDownReleased();
    ClientMessage receiveLeftPressed();
    ClientMessage receiveLeftReleased();
    ClientMessage receiveRightPressed();
    ClientMessage receiveRightReleased();

    ServerMessage receivePositionsUpdate();

public:
    explicit Protocol(Socket &&socket) noexcept; // constructor que toma ownership del socket
    Protocol() = delete;
    ~Protocol() = default;
    Protocol(const Protocol &) = delete;
    Protocol &operator=(const Protocol &) = delete;

    // Recibe mensaje del socket en formato DecodedMessage
    ClientMessage receiveClientMessage();
    ServerMessage receiveServerMessage();

    // Envía mensaje al socket
    void sendMessage(ServerMessage& out);
    void sendMessage(ClientMessage& out);

    void shutdown();
};

#endif
