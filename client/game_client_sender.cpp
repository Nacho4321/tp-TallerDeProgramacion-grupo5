#include "game_client_sender.h"
#include <iostream>

GameClientSender::GameClientSender(Protocol& proto, Queue<std::string>& messages) :
    protocol(proto), outgoing_messages(messages) {}

void GameClientSender::run() {
    try {
        while (should_keep_running()) {
            std::string msg;
            try {
                msg = outgoing_messages.pop();
            } catch (const ClosedQueue&) {
                break;
            }
            if (!should_keep_running()) break;
            ClientMessage client_msg;
            client_msg.cmd = msg;
            client_msg.player_id = player_id;
            client_msg.game_id = game_id;

            if (client_msg.cmd.rfind(JOIN_GAME_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string id_str = client_msg.cmd.substr(sp + 1);
                    try {
                        int parsed = std::stoi(id_str);
                        client_msg.game_id = parsed;
                    } catch (...) {
                    }
                }
                client_msg.cmd = JOIN_GAME_STR;
            }
            if (client_msg.cmd.rfind(CREATE_GAME_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string payload = client_msg.cmd.substr(sp + 1);
                    size_t pipe_pos = payload.find('|');
                    if (pipe_pos != std::string::npos) {
                        client_msg.game_name = payload.substr(0, pipe_pos);
                        try {
                            client_msg.map_id = static_cast<uint8_t>(std::stoi(payload.substr(pipe_pos + 1)));
                        } catch (...) {
                            client_msg.map_id = 0;
                        }
                    } else {
                        client_msg.game_name = payload;
                        client_msg.map_id = 0;
                    }
                    client_msg.cmd = CREATE_GAME_STR;
                } else {
                    client_msg.game_name = "";
                    client_msg.map_id = 0;
                }
            }
            if (client_msg.cmd.rfind(CHANGE_CAR_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    client_msg.car_type = client_msg.cmd.substr(sp + 1);
                }
            }

            if (client_msg.cmd.rfind(UPGRADE_CAR_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string upgrade_str = client_msg.cmd.substr(sp + 1);
                    try {
                        int upgrade_val = std::stoi(upgrade_str);
                        client_msg.upgrade_type = static_cast<CarUpgrade>(upgrade_val);
                    } catch (...) {
                        client_msg.upgrade_type = CarUpgrade::ACCELERATION_BOOST;
                        std::cout << "[Sender] UPGRADE_CAR parse failed, using default ACCELERATION_BOOST" << std::endl;
                    }
                }
            }
            if (client_msg.cmd == CHEAT_GOD_MODE_STR) {
                client_msg.cheat_type = CheatType::GOD_MODE;
            } else if (client_msg.cmd == CHEAT_DIE_STR) {
                client_msg.cheat_type = CheatType::DIE;
            } else if (client_msg.cmd == CHEAT_SKIP_LAP_STR) {
                client_msg.cheat_type = CheatType::SKIP_LAP;
            } else if (client_msg.cmd == CHEAT_FULL_UPGRADE_STR) {
                client_msg.cheat_type = CheatType::FULL_UPGRADE;
            }
            if (client_msg.cmd == GET_GAMES_STR) {
                // no payload extra
            }
            protocol.sendMessage(client_msg);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Game Client Sender] Exception: " << e.what() << std::endl;
        if (should_keep_running()) {
            throw;  
        }
    }
}

void GameClientSender::stop() {
    Thread::stop(); 
    outgoing_messages.close(); 
}

void GameClientSender::set_player_id(int32_t id) {
    player_id = id;
}

void GameClientSender::set_game_id(int32_t id) {
    game_id = id;
}
