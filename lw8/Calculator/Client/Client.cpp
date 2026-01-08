#include "Client.h"
#include "../SocketGuard.h"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

constexpr int RECONNECT_ATTEMPT = 5;

int ConnectToServer(const std::string& host, int port);
std::string TryGetResult(
	const std::string& host,
	int port,
	SocketGuard& sock,
	const std::string& input);

void RunClient(const std::string& host, const int port)
{
	SocketGuard sock(ConnectToServer(host, port));
	if (sock < 0)
	{
		throw std::runtime_error("Connection failed");
	}

	std::string input;
	while (std::getline(std::cin, input))
	{
		if (input.empty())
		{
			continue;
		}

		auto res = TryGetResult(host, port, sock, input);
		std::cout << "Result: " << res;
	}
}

std::string TryGetResult(
	const std::string& host,
	const int port,
	SocketGuard& sock,
	const std::string& input)
{
	for (int attempt = 0; attempt < RECONNECT_ATTEMPT; attempt++)
	{
		constexpr int BUFFER_SIZE = 1024;
		if (send(sock, (input + "\n").c_str(), input.length() + 1, 0) < 0)
		{
			std::cout << "Reconnecting..." << std::endl;
			sock.Reset(ConnectToServer(host, port));
			if (!sock.IsValid())
			{
				usleep(500000);
			}
			continue;
		}

		char buffer[BUFFER_SIZE];
		const auto n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
		if (n > 0)
		{
			buffer[n] = '\0';
			return buffer;
		}
		if (n == 0)
		{
			sock.Reset();
		}
	}
	throw std::runtime_error("Connection lost. Exiting");
}

int ConnectToServer(const std::string& host, const int port)
{
	const int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	inet_pton(AF_INET, host.c_str(), &address.sin_addr);

	if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
	{
		close(sock);
		return -1;
	}
	return sock;
}
