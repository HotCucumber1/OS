#pragma once
#include "Room.h"

#include <boost/asio/ip/tcp.hpp>

class Server
{
public:
	Server(
		boost::asio::io_context& ioContext,
		int port);

private:
	void Accept();

private:
	boost::asio::ip::tcp::acceptor m_acceptor;
	Room m_room;
};
