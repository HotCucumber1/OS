#pragma once

#include "../Segment.h"
#include <memory>
#include <vector>

class Session;

class Room
{
public:
	void Join(const std::shared_ptr<Session>& session);

	void Leave(const std::shared_ptr<Session>& session);

	void Deliver(const Segment& data);

private:
	std::vector<std::shared_ptr<Session>> m_sessions;
	std::vector<Segment> m_history;
};
