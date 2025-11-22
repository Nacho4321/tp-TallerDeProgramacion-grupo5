#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>

const std::uint8_t MOVE_UP_PRESSED = 0x01;
const std::uint8_t MOVE_UP_RELEASED = 0x02;
const std::uint8_t MOVE_DOWN_PRESSED = 0x03;
const std::uint8_t MOVE_DOWN_RELEASED = 0x04;
const std::uint8_t MOVE_LEFT_PRESSED = 0x05;
const std::uint8_t MOVE_LEFT_RELEASED = 0x06;
const std::uint8_t MOVE_RIGHT_PRESSED = 0x07;
const std::uint8_t MOVE_RIGHT_RELEASED = 0x08;

// Lobby opcodes
const std::uint8_t CREATE_GAME = 0x10;
const std::uint8_t JOIN_GAME = 0x11;
const std::uint8_t GAME_JOINED = 0x12;
// Listar partidas existentes
const std::uint8_t GET_GAMES = 0x13;
const std::uint8_t GAMES_LIST = 0x14;

// Game opcodes
const std::uint8_t UPDATE_POSITIONS = 0x20;

const int OPCODE_SIZE = 1;
const std::uint8_t POSITIONS_SIZE = 2;

// Radio de los checkpoints en píxeles que usan servidor y cliente para dibujar
constexpr float CHECKPOINT_RADIUS_PX = 20.0f;

// Display/Rendering constants - logical screen dimensions for aspect ratio preservation
constexpr int LOGICAL_SCREEN_WIDTH = 854;
constexpr int LOGICAL_SCREEN_HEIGHT = 480;

const std::string MOVE_UP_PRESSED_STR = "move_up_pressed";              // NOLINT
const std::string MOVE_UP_RELEASED_STR = "move_up_released";            // NOLINT
const std::string MOVE_DOWN_PRESSED_STR = "move_down_pressed";          // NOLINT
const std::string MOVE_DOWN_RELEASED_STR = "move_down_released";        // NOLINT
const std::string MOVE_LEFT_PRESSED_STR = "move_left_pressed";            // NOLINT
const std::string MOVE_LEFT_RELEASED_STR = "move_left_released";          // NOLINT
const std::string MOVE_RIGHT_PRESSED_STR = "move_right_pressed";          // NOLINT
const std::string MOVE_RIGHT_RELEASED_STR = "move_right_released";        // NOLINT

// Lobby commands
const std::string CREATE_GAME_STR = "create_game";                        // NOLINT
const std::string JOIN_GAME_STR = "join_game";                            // NOLINT
const std::string GET_GAMES_STR = "get_games";                            // NOLINT
const std::string LEAVE_GAME_STR = "leave_game";                          // NOLINT


#endif
