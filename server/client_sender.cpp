#include "client_sender.h"

#include <iostream>

ClientSender::ClientSender(Protocol &proto, Queue<ServerMessage> &ob)
	: protocol(proto), outbox(ob) {}

void ClientSender::run()
{
	try
	{
		while (should_keep_running())
		{
			ServerMessage response;
			try
			{
				response = outbox.pop(); // bloqueante
			}
			catch (const ClosedQueue &)
			{
				// La cola fue cerrada: salimos del loop
				break;
			}

			// Enviar directamente el mensaje unificado
			protocol.sendMessage(response);
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "[Sender] Exception: " << e.what() << std::endl;
	}
}
