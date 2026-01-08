#pragma once
#include "../Segment.h"

#include <boost/asio/ip/tcp.hpp>
#include <deque>

class Room;

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(
		boost::asio::ip::tcp::socket socket,
		Room& room);

	void Start();

	void Deliver(const Segment& msg);

	void Read();

	void Write();

private:
	boost::asio::ip::tcp::socket m_socket;
	Segment m_readData{};
	Room& m_room;
	std::deque<Segment> m_writeData;
};
