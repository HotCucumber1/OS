#include "App/App.h"
#include "Client/Client.h"
#include "Server/Server.h"

#include <boost/asio/io_context.hpp>
#include <iostream>

void RunServer(const std::string& portStr);
void RunClient(const std::string& host, const std::string& port);

int main(const int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage (Server mode): whiteboard <PORT>" << std::endl;
		std::cerr << "Usage (Client mode): whiteboard <ADDRESS> <PORT>" << std::endl;
		return 1;
	}

	try
	{
		if (argc == 2)
		{
			RunServer(argv[1]);
		}
		else
		{
			RunClient(argv[1], argv[2]);
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}

void RunServer(const std::string& portStr)
{
	const int port = std::stoi(portStr);
	boost::asio::io_context ioContext;
	Server server(ioContext, port);

	std::jthread serverNetThread([&ioContext]() {
		ioContext.run();
	});

	std::cout << "Server started on port " << port << ". Launching GUI..." << std::endl;

	Client client("localhost", portStr);
	App app(800, 600, "Whiteboard - Server Mode", client);
	app.Run();

	ioContext.stop();
}

void RunClient(const std::string& host, const std::string& port)
{
	std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;

	Client client(host, port);

	App app(800, 600, "Whiteboard - Client Mode", client);
	app.Run();
}
