#include "game_client_sender.h"
#include <iostream>

GameClientSender::GameClientSender(Protocol& proto, Queue<std::string>& messages) :
    protocol(proto), outgoing_messages(messages) {}

void GameClientSender::run() {
    try {
        while (should_keep_running()) {
            std::string msg;
            try {
                msg = outgoing_messages.pop();  // bloqueante
            } catch (const ClosedQueue&) {
                // La cola fue cerrada: salimos del loop
                break;
            }
            if (!should_keep_running()) break;  // Verificar antes de enviar
            ClientMessage client_msg;
            client_msg.cmd = msg;
            // Adaptación al nuevo protocolo: incluir IDs
            client_msg.player_id = player_id;
            client_msg.game_id = game_id;

            // Fallback: si es join_game con id en el string y aún no tenemos game_id seteado,
            // parsear el id del comando y usarlo en este envío.
            if (client_msg.cmd.rfind(JOIN_GAME_STR, 0) == 0) {
                // buscar un espacio y parsear el número siguiente
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string id_str = client_msg.cmd.substr(sp + 1);
                    try {
                        int parsed = std::stoi(id_str);
                        client_msg.game_id = parsed;
                    } catch (...) {
                        // si falla, dejamos el game_id como está
                    }
                }
                // Normalizar el comando a solo "join_game" para el mapeo de opcode
                client_msg.cmd = JOIN_GAME_STR;
            }
            // Parse create_game <nombre>|<map_id>
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
                    client_msg.cmd = CREATE_GAME_STR; // normalizar
                } else {
                    client_msg.game_name = ""; // nombre vacío => server asigna por defecto
                    client_msg.map_id = 0;
                }
            }
            // Parse change_car <tipo>
            if (client_msg.cmd.rfind(CHANGE_CAR_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    client_msg.car_type = client_msg.cmd.substr(sp + 1);
                    // Keep full cmd (with tipo) for server event visibility, but protocol mapping uses base + payload
                    // Optionally normalize to base command only for opcode mapping logic if needed
                }
            }
            // Parse upgrade_car <tipo>
            if (client_msg.cmd.rfind(UPGRADE_CAR_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string upgrade_str = client_msg.cmd.substr(sp + 1);
                    try {
                        int upgrade_val = std::stoi(upgrade_str);
                        client_msg.upgrade_type = static_cast<CarUpgrade>(upgrade_val);
                        std::cout << "[Sender] Parsed UPGRADE_CAR: upgrade_str='" << upgrade_str 
                                  << "' upgrade_val=" << upgrade_val << std::endl;
                    } catch (...) {
                        client_msg.upgrade_type = CarUpgrade::ACCELERATION_BOOST; // default
                        std::cout << "[Sender] UPGRADE_CAR parse failed, using default ACCELERATION_BOOST" << std::endl;
                    }
                } else {
                    std::cout << "[Sender] UPGRADE_CAR sin argumento, cmd='" << client_msg.cmd << "'" << std::endl;
                }
            }
            // Parse cheats - mapear comando a CheatType
            if (client_msg.cmd == CHEAT_GOD_MODE_STR) {
                client_msg.cheat_type = CheatType::GOD_MODE;
                std::cout << "[Sender] Sending CHEAT: GOD_MODE" << std::endl;
            } else if (client_msg.cmd == CHEAT_DIE_STR) {
                client_msg.cheat_type = CheatType::DIE;
                std::cout << "[Sender] Sending CHEAT: DIE" << std::endl;
            } else if (client_msg.cmd == CHEAT_SKIP_LAP_STR) {
                client_msg.cheat_type = CheatType::SKIP_LAP;
                std::cout << "[Sender] Sending CHEAT: SKIP_LAP" << std::endl;
            } else if (client_msg.cmd == CHEAT_FULL_UPGRADE_STR) {
                client_msg.cheat_type = CheatType::FULL_UPGRADE;
                std::cout << "[Sender] Sending CHEAT: FULL_UPGRADE" << std::endl;
            }
            if (client_msg.cmd == GET_GAMES_STR) {
                // no payload extra
            }
            protocol.sendMessage(client_msg);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Game Client Sender] Exception: " << e.what() << std::endl;
        if (should_keep_running()) {
            throw;  // Solo propagamos el error si no estábamos parando el hilo
        }
    }
}

void GameClientSender::stop() {
    Thread::stop(); 
    outgoing_messages.close();  // Cierra la cola para interrumpir pop()
}

void GameClientSender::set_player_id(int32_t id) {
    player_id = id;
}

void GameClientSender::set_game_id(int32_t id) {
    game_id = id;
}
