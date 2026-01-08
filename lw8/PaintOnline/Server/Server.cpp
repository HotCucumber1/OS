#include "Server.h"
#include "Session.h"

Server::Server(boost::asio::io_context& ioContext, const int port)
	: m_acceptor(
		  ioContext,
		  boost::asio::ip::tcp::endpoint(
			  boost::asio::ip::tcp::v4(),
			  port))
{
	Accept();
}

void Server::Accept()
{
	m_acceptor.async_accept([this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
		if (!ec)
		{
			std::make_shared<Session>(std::move(socket), m_room)->Start();
		}
		Accept();
	});
}
