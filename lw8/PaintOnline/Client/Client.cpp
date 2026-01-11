#include "Client.h"
#include "../Segment.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

Client::Client(
	const std::string& host,
	const std::string& port)
	: m_socket(m_ioContext)
{
	boost::asio::ip::tcp::resolver resolver(m_ioContext);
	boost::asio::connect(m_socket, resolver.resolve(host, port));

	m_netThread = std::jthread([this](const std::stop_token& st) {
		RunNetwork(st);
	});
}

Client::~Client()
{
	Stop();
}

void Client::RunNetwork(const std::stop_token& st)
{
	try
	{
		while (!st.stop_requested())
		{
			Segment data{};
			boost::asio::read(
				m_socket,
				boost::asio::buffer(&data, sizeof(Segment)));

			std::lock_guard lock(m_mutex);
			m_drawData.push_back(data);
		}
	}
	catch (const std::exception&)
	{
	}
}

void Client::SendSegment(
	const int x1,
	const int y1,
	const int x2,
	const int y2,
	const uint32_t color)
{
	Segment s{ x1, y1, x2, y2, color };
	boost::asio::write(
		m_socket,
		boost::asio::buffer(&s, sizeof(Segment)));
}

std::vector<Segment> Client::GetData()
{
	std::lock_guard lock(m_mutex);
	return m_drawData;
}

void Client::Stop()
{
	boost::system::error_code ec;
	m_socket.close(ec);
}
