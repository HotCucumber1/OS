#include "Session.h"

#include "Room.h"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

Session::Session(boost::asio::ip::tcp::socket socket, Room& room)
	: m_socket(std::move(socket))
	, m_room(room)
{
}

void Session::Start()
{
	m_room.Join(shared_from_this()); // TODO когда разрушается
	Read();
}

void Session::Deliver(const Segment& msg)
{
	bool writeInProgress = !m_writeData.empty();
	m_writeData.push_back(msg);
	if (!writeInProgress)
	{
		Write();
	}
}

void Session::Read()
{
	auto self = shared_from_this();
	boost::asio::async_read(
		m_socket,
		boost::asio::buffer(&m_readData, sizeof(Segment)),
		[this, self](const boost::system::error_code& ec, size_t) {
			if (!ec)
			{
				m_room.Deliver(m_readData);
				Read();
			}
			else
			{
				m_room.Leave(self);
			}
		});
}

void Session::Write()
{
	auto self = shared_from_this();
	boost::asio::async_write(
		m_socket,
		boost::asio::buffer(&m_writeData.front(), sizeof(Segment)),
		[this, self](const boost::system::error_code& ec, std::size_t) {
			if (!ec)
			{
				m_writeData.pop_front();
				if (!m_writeData.empty())
				{
					Write();
				}
			}
			else
			{
				m_room.Leave(self);
			}
		});
}