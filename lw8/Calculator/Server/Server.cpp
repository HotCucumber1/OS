#include "Server.h"
#include "../SocketGuard.h"
#include "Calculator.h"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <netinet/in.h>
#include <sys/socket.h>

constexpr int QUEUE_LENGTH = 5;
constexpr int THREADS_NUM = 8;

void Handle(int clientSockFd);
void SendClientMessage(int sock, const std::string& msg);

void RunServer(const int port, std::ostream& output)
{
	const SocketGuard server(socket(AF_INET, SOCK_STREAM, 0));
	if (!server.IsValid())
	{
		throw std::runtime_error("Can't create socket");
	}

	int opt = 1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
	{
		throw std::runtime_error("Port " + std::to_string(port) + " is already in use");
	}
	listen(server, QUEUE_LENGTH);
	output << "Server started on port " << port << std::endl;

	boost::asio::thread_pool pool(THREADS_NUM);
	while (true)
	{
		int clientSock = accept(server, nullptr, nullptr);
		if (clientSock < 0)
		{
			continue;
		}
		boost::asio::post(pool, [clientSock]() {
			Handle(clientSock);
		});
	}
}

void Handle(const int clientSockFd)
{
	constexpr int BUFFER_SIZE = 1024;
	const SocketGuard clientSock(clientSockFd);
	char buffer[BUFFER_SIZE];
	std::string data;

	while (true)
	{
		const auto bytesReceived = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
		if (bytesReceived <= 0)
		{
			break;
		}

		buffer[bytesReceived] = '\0';
		data += buffer;

		auto pos = data.find('\n');
		while (pos != std::string::npos)
		{
			auto command = data.substr(0, pos);
			data.erase(0, pos + 1);

			if (command.empty())
			{
				continue;
			}
			try
			{
				const auto result = ProcessCommand(command);
				SendClientMessage(clientSock, "OK " + std::to_string(result));
			}
			catch (const std::exception& e)
			{
				SendClientMessage(clientSock, e.what());
			}
			pos = data.find('\n');
		}
	}
}

void SendClientMessage(const int sock, const std::string& msg)
{
	const std::string data = msg + "\n";
	send(
		sock,
		data.c_str(),
		data.length(),
		0);
}
