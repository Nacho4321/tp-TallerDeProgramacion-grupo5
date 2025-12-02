#include "client_sender.h"

#include <iostream>

ClientSender::ClientSender(Protocol &proto, Queue<ServerMessage> &ob)
	: protocol(proto), outbox(ob) {}

void ClientSender::run()
{
	std::cout << "[ClientSender] Hilo sender iniciado" << std::endl;
	try
	{
		while (should_keep_running())
		{
			ServerMessage response;
			try
			{
				response = outbox.pop(); // bloqueante
				std::cout << "[ClientSender] Pop de outbox: opcode 0x" << std::hex << (int)response.opcode << std::dec << std::endl;
			}
			catch (const ClosedQueue &)
			{
				std::cout << "[ClientSender] Cola cerrada, saliendo" << std::endl;
				// La cola fue cerrada: salimos del loop
				break;
			}

			// Enviar directamente el mensaje unificado
			std::cout << "[ClientSender] Enviando mensaje..." << std::endl;
			protocol.sendMessage(response);
			std::cout << "[ClientSender] Mensaje enviado" << std::endl;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "[Sender] Exception: " << e.what() << std::endl;
	}
}
