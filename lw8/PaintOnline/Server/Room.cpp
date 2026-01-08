#include "Room.h"

#include "Session.h"

void Room::Join(const std::shared_ptr<Session>& session)
{
	m_sessions.push_back(session);
	for (const auto& data : m_history)
	{
		session->Deliver(data);
	}
}

void Room::Leave(const std::shared_ptr<Session>& session)
{
	std::erase(m_sessions, session);
}

void Room::Deliver(const Segment& data)
{
	m_history.push_back(data);
	for (const auto& session : m_sessions)
	{
		session->Deliver(data);
	}
}