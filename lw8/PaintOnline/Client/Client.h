#pragma once
#include "../Segment.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <thread>

class Client
{
public:
	Client(const std::string& host, const std::string& port);

	~Client();

	void SendSegment(int x1, int y1, int x2, int y2, uint32_t color);

	std::vector<Segment> GetData();

	void Stop();

private:
	void RunNetwork(const std::stop_token& st);

private:
	boost::asio::io_context m_ioContext;
	boost::asio::ip::tcp::socket m_socket;
	std::vector<Segment> m_drawData;
	std::mutex m_mutex;
	std::jthread m_netThread;
};
