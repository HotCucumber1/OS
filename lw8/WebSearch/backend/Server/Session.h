#pragma once

#include "../RequestHandler.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/string_body.hpp>
#include <memory>

class Session : public std::enable_shared_from_this<Session>
{
public:
	explicit Session(
		boost::asio::ip::tcp::socket socket,
		const std::shared_ptr<RequestHandler>& handler);

	void Start();

private:
	void Read();

	void OnRead(const boost::beast::error_code& errorCode, std::size_t bytesRead);

	void SendResponse(http::message_generator&& message);

	void Close();

private:
	std::shared_ptr<RequestHandler> m_handler;
	boost::beast::tcp_stream m_stream;
	boost::beast::flat_buffer m_buffer;
	http::request<http::string_body> m_request;
};
