#include "backend/RequestHandler.h"
#include "backend/Server/Listener.h"

#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <memory>

int main(const int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: ./WebSearch <ADDRESS> <PORT>" << std::endl;
		return 1;
	}

	try
	{
		const auto address = boost::asio::ip::make_address(argv[1]);
		const unsigned short port = std::stoi(argv[2]);

		const auto handler = std::make_shared<RequestHandler>();

		boost::asio::io_context ioContext;

		const auto listener = std::make_shared<Listener>(
			ioContext,
			boost::asio::ip::tcp::endpoint{ address, port },
			handler);

		boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
		signals.async_wait([&](const boost::system::error_code& ec, int signal_number) {
			std::cout << "\nStopping server..." << std::endl;
			listener->Stop();
		});

		listener->Run();
		std::cout << "Server started on port " << port << std::endl;
		ioContext.run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}
