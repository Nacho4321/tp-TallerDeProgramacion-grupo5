#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "constants.h"
#include "messages.h"
#include "socket.h"

class Protocol
{
private:
    Socket skt;
    std::vector<uint8_t> buffer;
    std::vector<uint8_t> readBuffer;

    using ClientMessageHandler = std::function<ClientMessage()>;
    std::unordered_map<uint8_t, ClientMessageHandler> receive_handlers;
    
    using ServerEncodeHandler = std::function<void(ServerMessage&)>;
    std::unordered_map<uint8_t, ServerEncodeHandler> server_encode_handlers;
    
    using ClientEncodeHandler = std::function<void(const ClientMessage&, uint8_t)>;
    std::unordered_map<uint8_t, ClientEncodeHandler> client_encode_handlers;
    
    using ServerReceiveHandler = std::function<void(ServerMessage&, GameJoinedResponse&)>;
    std::unordered_map<uint8_t, ServerReceiveHandler> server_receive_handlers;
    
    using CmdToOpcodeMap = std::unordered_map<std::string, uint8_t>;
    CmdToOpcodeMap cmd_to_opcode;
    
    void init_handlers();
    void init_cmd_map();
    void init_encode_handlers();
    void init_server_receive_handlers();


    template <typename T>
    void appendValue(T value)
    {
        size_t old_size = buffer.size();
        buffer.resize(old_size + sizeof(T));
        std::memcpy(buffer.data() + old_size, &value, sizeof(T));
    }

    template <typename T>
    T readValue(const std::vector<uint8_t> &buffer, size_t &idx)
    {
        T value;
        std::memcpy(&value, buffer.data() + idx, sizeof(T));
        idx += sizeof(T);
        return value;
    }

    void insertUint16(std::uint16_t value);
    void insertUint32(std::uint32_t value);
    void insertFloat(float value);
    void insertInt(int value);
    void insertString(const std::string& str);
    void insertPosition(const Position& pos);

    uint16_t exportUint16(const std::vector<uint8_t> &buffer, size_t &idx);
    uint32_t exportUint32(const std::vector<uint8_t> &buffer, size_t &idx);
    float exportFloat(const std::vector<uint8_t> &buffer, size_t &idx);
    int exportInt(const std::vector<uint8_t> &buffer, size_t &idx);

    void readClientIds(ClientMessage& msg);
    
    bool readPosition(Position& pos);
    bool readString(std::string& str);
    bool readPlayerPositionUpdate(PlayerPositionUpdate& update);

    std::vector<std::uint8_t> encodeClientMessage(const ClientMessage &msg);
    std::vector<std::uint8_t> encodeServerMessage(ServerMessage& out);
    std::vector<std::uint8_t> encodeGameJoinedResponse(const GameJoinedResponse& response);

    std::vector<std::uint8_t> encodeOpcode(std::uint8_t opcode);

    void encodeUpdatePositions(ServerMessage& out);
    void encodeGameJoined(ServerMessage& out);
    void encodeGamesList(ServerMessage& out);
    void encodeRaceTimes(ServerMessage& out);
    void encodeTotalTimes(ServerMessage& out);
    void encodeDefaultOpcode(ServerMessage& out);
    
    void encodeCreateGame(const ClientMessage& msg);
    void encodeChangeCar(const ClientMessage& msg);
    void encodeUpgrade(const ClientMessage& msg);
    void encodeCheat(const ClientMessage& msg);

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
    ClientMessage receiveGetGames();
    ClientMessage receiveStartGame();
    ClientMessage receiveChangeCar();
    ClientMessage receiveUpgradeCar();
    ClientMessage receiveCheat();

    ServerMessage receivePositionsUpdate();
    ServerMessage receiveGamesList();
    GameJoinedResponse receiveGameJoinedResponse();
    ServerMessage receiveRaceTimes();
    ServerMessage receiveTotalTimes();

    ServerMessage receiveStartingCountdown();

public:
    explicit Protocol(Socket &&socket) noexcept;
    Protocol() = delete;
    ~Protocol() = default;
    Protocol(const Protocol &) = delete;
    Protocol &operator=(const Protocol &) = delete;

    // Recibe mensaje del socket en formato DecodedMessage
    ClientMessage receiveClientMessage();
    // Lee el próximo paquete del servidor y devuelve true si pudo decodificar alguno.
    // outOpcode indicará el tipo (p.ej. UPDATE_POSITIONS o GAME_JOINED).
    bool receiveAnyServerPacket(ServerMessage& outServer,
                                GameJoinedResponse& outJoined,
                                uint8_t& outOpcode);
    
    void sendMessage(ServerMessage& out);
    void sendMessage(ClientMessage& out);
    void sendMessage(const GameJoinedResponse& response);

    void shutdown();
};

#endif
