#include "Session.h"

#include <boost/beast/http/read.hpp>

constexpr int TIMEOUT_IN_SECONDS = 30;

Session::Session(
	boost::asio::ip::tcp::socket socket,
	const std::shared_ptr<RequestHandler>& handler)
	: m_handler(handler)
	, m_stream(std::move(socket))
{
}

void Session::Start()
{
	Read();
}

void Session::Read()
{
	m_request = {};
	m_stream.expires_after(std::chrono::seconds(TIMEOUT_IN_SECONDS));
	http::async_read(
		m_stream,
		m_buffer,
		m_request,
		boost::beast::bind_front_handler(&Session::OnRead, shared_from_this()));
}

void Session::OnRead(const boost::beast::error_code& errorCode, std::size_t bytesRead)
{
	if (errorCode == http::error::end_of_stream)
	{
		Close();
		return;
	}
	if (errorCode)
	{
		return;
	}

	try
	{
		SendResponse(m_handler->Handle(std::move(m_request)));
	}
	catch (const std::exception& e)
	{
		Close();
	}
}

void Session::SendResponse(http::message_generator&& message)
{
	auto keepAlive = message.keep_alive();
	auto self = shared_from_this();
	boost::beast::async_write(
		m_stream,
		std::move(message),
		[self, keepAlive](const boost::beast::error_code& ec, std::size_t) {
			if (!ec && keepAlive)
			{
				self->Read();
			}
			else
			{
				self->Close();
			}
		});
}

void Session::Close()
{
	boost::beast::error_code errorCode;
	m_stream.socket().shutdown(
		boost::asio::ip::tcp::socket::shutdown_send,
		errorCode);
}
