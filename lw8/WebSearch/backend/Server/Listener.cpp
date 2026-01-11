#include "Listener.h"
#include "Session.h"

#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>

Listener::Listener(
	boost::asio::io_context& ioContext,
	const boost::asio::ip::tcp::endpoint& endpoint,
	const std::shared_ptr<RequestHandler>& handler)

	: m_ioContext(ioContext)
	, m_acceptor(boost::asio::make_strand(ioContext))
	, m_handler(handler)
{
	boost::beast::error_code errorCode;
	m_acceptor.open(endpoint.protocol(), errorCode);
	m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	m_acceptor.bind(endpoint, errorCode);
	m_acceptor.listen(boost::asio::socket_base::max_connections, errorCode);
}

void Listener::Run()
{
	Accept();
}

void Listener::Accept()
{
	m_acceptor.async_accept(
		boost::asio::make_strand(m_ioContext),
		boost::beast::bind_front_handler(&Listener::OnAccept, shared_from_this()));
}

void Listener::OnAccept(
	const boost::beast::error_code& errorCode,
	boost::asio::ip::tcp::socket socket)
{
	if (!errorCode)
	{
		std::make_shared<Session>(std::move(socket), std::move(m_handler))->Start();
	}
	Accept();
}