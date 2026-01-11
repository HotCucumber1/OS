#pragma once
#include "../RequestHandler.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>
#include <memory>

class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(
		boost::asio::io_context& ioContext,
		const boost::asio::ip::tcp::endpoint& endpoint,
		const std::shared_ptr<RequestHandler>& handler);

	void Run();

	void Stop();

private:
	void Accept();

	void OnAccept(
		const boost::beast::error_code& errorCode,
		boost::asio::ip::tcp::socket socket);

private:
	boost::asio::io_context& m_ioContext;
	boost::asio::ip::tcp::acceptor m_acceptor;
	std::shared_ptr<RequestHandler> m_handler;
};
